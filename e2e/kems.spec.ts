import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import yaml from 'js-yaml';
import { test, expect, type Page } from '@playwright/test';

// Real data rows carry a title attribute on the Parameter Set cell (4th column);
// the "No KEMs to display" fallback row has a single colspan cell and never
// matches, so these locators naturally exclude it without any literal counts.
const paramSetCellSelector = 'tbody tr td:nth-child(4)[title]';
const levelCellSelector = 'tbody tr td:nth-child(5)';

// Expected counts come from data/kems/*.yaml directly (the source of truth), not
// from the rendered page — asserting DOM against DOM would be tautological and
// wouldn't catch a rendering/filtering bug that drops or duplicates rows.
const KEMS_DIR = path.join(path.dirname(fileURLToPath(import.meta.url)), '..', 'data', 'kems');

interface SourceKem {
	programmes?: Record<string, unknown>;
	broken?: string;
	parametersets: {
		level: unknown;
		recommended?: boolean;
		encaps_us?: number;
		encaps_cycles?: number;
	}[];
}

function loadSourceKems(): SourceKem[] {
	return fs
		.readdirSync(KEMS_DIR)
		.filter((f) => f.endsWith('.yaml'))
		.map((file) => yaml.load(fs.readFileSync(path.join(KEMS_DIR, file), 'utf8')) as SourceKem);
}

const sourceKems = loadSourceKems();
// The default view shows recommended parameter sets: if a KEM flags any, only those.
const shownSets = (k: SourceKem) =>
	k.parametersets.some((p) => p.recommended) ? k.parametersets.filter((p) => p.recommended) : k.parametersets;
const shownParameterSets = sourceKems.flatMap(shownSets);
const expectedTotal = shownParameterSets.length;
const expectedNaCount = shownParameterSets.filter((ps) => ps.level === 'Pre-Quantum').length;
const researchCount = sourceKems
	.filter((k) => !k.programmes && k.broken !== 'classical')
	.reduce((n, k) => n + shownSets(k).length, 0);
const unbenchmarkedEncaps = shownParameterSets.filter((ps) => ps.encaps_us == null && !(ps.encaps_cycles! > 0)).length;

async function paramSetCount(page: Page): Promise<number> {
	const text = await page.getByText(/^\d+ parameter sets?$/).innerText();
	const match = text.match(/(\d+)/);
	if (!match) throw new Error(`could not parse count from "${text}"`);
	return parseInt(match[1], 10);
}

async function levelTexts(page: Page): Promise<string[]> {
	return page.$$eval(levelCellSelector, (cells) => cells.map((c) => c.textContent?.trim() ?? ''));
}

test.describe('KEMs page', () => {
	test('loads with correct heading', async ({ page }) => {
		await page.goto('/kems/');
		await expect(page.locator('h1')).toHaveText('Key Encapsulation Mechanisms');
	});

	test('reachable via the header nav link', async ({ page }) => {
		await page.goto('/signatures/');
		// Wait for hydration so the click is an SPA navigation, not a full reload
		// that can time out under load.
		await page.waitForLoadState('networkidle');
		await page.getByRole('link', { name: 'KEMs' }).first().click();
		await expect(page.locator('h1')).toHaveText('Key Encapsulation Mechanisms', {
			timeout: 15_000
		});
	});

	test('header links to Signatures, KEMs and Timeline', async ({ page }) => {
		await page.goto('/kems/');
		const nav = page.getByRole('navigation').first();
		await expect(nav.getByRole('link', { name: 'Signatures', exact: true })).toHaveAttribute('href', /\/signatures\/$/);
		await expect(nav.getByRole('link', { name: 'KEMs', exact: true })).toHaveAttribute('aria-current', 'page');
		await expect(nav.getByRole('link', { name: 'Timeline', exact: true })).toHaveAttribute('href', /\/timeline\/$/);
	});

	test('Vega scatter plot renders', async ({ page }) => {
		await page.goto('/kems/');
		await page.waitForFunction(
			() => [...document.querySelectorAll('svg')].some((s) => s.querySelector('g')),
			{ timeout: 15_000 }
		);
		const vegaSvg = page.locator('svg').filter({ has: page.locator('g') }).first();
		await expect(vegaSvg).toBeVisible();
	});

	test('scatter plot section heading', async ({ page }) => {
		await page.goto('/kems/');
		await expect(page.locator('section h2').first()).toContainText('pk size vs. ciphertext size');
	});

	test('table lists all KEM parameter sets', async ({ page }) => {
		await page.goto('/kems/');
		await expect(page.locator('table')).toBeVisible();

		const rowCount = await page.locator(paramSetCellSelector).count();
		expect(rowCount).toBe(expectedTotal);
		expect(await paramSetCount(page)).toBe(expectedTotal);

		// The parameter-set cell may display a shortened name (scheme prefix
		// stripped), but always carries the full name as its title attribute.
		const cells = await page.$$eval(paramSetCellSelector, (els) =>
			els.map((el) => ({ title: el.getAttribute('title') ?? '', text: el.textContent?.trim() ?? '' }))
		);
		for (const { title, text } of cells) {
			expect(title.length).toBeGreaterThan(0);
			expect(title.endsWith(text)).toBeTruthy();
		}
	});

	test('table has benchmark columns and the env panel is present', async ({ page }) => {
		await page.goto('/kems/');
		for (const col of ['Keygen', 'Encaps', 'Decaps']) {
			await expect(page.getByRole('columnheader', { name: new RegExp(col) })).toBeVisible();
		}
		await expect(page.getByRole('cell', { name: /µs|ms/ }).first()).toBeVisible();
		await expect(page.getByRole('heading', { name: 'Benchmark environment' })).toBeVisible();
	});

	test('filter panel narrows the rows by scheme', async ({ page }) => {
		await page.goto('/kems/');
		// First aside in the DOM is the desktop sidebar (visible at the default viewport).
		const panel = page.locator('aside').first();
		await expect(panel.getByRole('heading', { name: 'Filters' })).toBeVisible();

		await expect(page.getByText(`${expectedTotal} parameter set`)).toBeVisible();

		await panel.getByRole('button', { name: 'None' }).click();
		await expect(page.getByText('0 parameter sets')).toBeVisible();

		await panel.getByRole('button', { name: 'All' }).click();
		await expect.poll(() => paramSetCount(page)).toBe(expectedTotal);
	});

	test('level filter excludes pre-quantum schemes', async ({ page }) => {
		await page.goto('/kems/');
		const panel = page.locator('aside').first();

		// Sanity check: the assertion below is meaningless if the source data has
		// nothing pre-quantum to exclude.
		expect(expectedNaCount).toBeGreaterThan(0);
		expect(await paramSetCount(page)).toBe(expectedTotal);

		await panel.getByLabel('Pre-quantum', { exact: true }).uncheck();

		await expect.poll(() => paramSetCount(page)).toBe(expectedTotal - expectedNaCount);
		expect(await levelTexts(page)).not.toContain('N/A');
	});

	test('persists filter state in the URL across reload', async ({ page }) => {
		await page.goto('/kems/');
		const panel = page.locator('aside').first();

		await panel.getByLabel('Pre-quantum', { exact: true }).uncheck();
		// debounced URL sync (300ms); toHaveURL polls until it matches
		await expect(page).toHaveURL(/[?&]l=/);

		await page.reload();
		// Filter is re-applied from the URL in onMount, after hydration — poll rather
		// than reading once, so we don't race the pre-hydration default-filter render.
		await expect.poll(() => paramSetCount(page)).toBe(expectedTotal - expectedNaCount);
		expect(await levelTexts(page)).not.toContain('N/A');
	});

	test('table has a Programmes column in place of Status', async ({ page }) => {
		await page.goto('/kems/');
		await expect(page.getByRole('columnheader', { name: 'Programmes' })).toBeVisible();
		await expect(page.locator('tbody')).toContainText('FIPS');
		await expect(page.locator('tbody')).toContainText('KpqC selected');
	});

	test('programme filter hides research proposals', async ({ page }) => {
		expect(researchCount).toBeGreaterThan(0);
		await page.goto('/kems/');
		await page.waitForLoadState('networkidle');
		const panel = page.locator('aside').first();
		await panel.getByLabel('Research proposals').uncheck();
		await expect.poll(() => paramSetCount(page)).toBe(expectedTotal - researchCount);
		await expect(page).toHaveURL(/[?&]p=/);
	});

	test('performance filter bounds encapsulation time and keeps unbenchmarked rows', async ({ page }) => {
		await page.goto('/kems/');
		await page.waitForLoadState('networkidle');
		const panel = page.locator('aside').first();
		await panel.getByLabel('Max encaps').fill('0.0001');
		await expect.poll(() => paramSetCount(page)).toBe(unbenchmarkedEncaps);
		await expect(page).toHaveURL(/[?&]euMax=/);
	});

	test('KEM filters have the same sections as the signatures page', async ({ page }) => {
		await page.goto('/kems/');
		for (const name of ['Programme', 'NIST category', 'NGCC target (classical bits)', 'Parameter sets', 'Security status']) {
			await expect(page.getByRole('group', { name })).toBeVisible();
		}
		await expect(page.getByRole('button', { name: 'Reset filters' })).toBeVisible();
	});
});
