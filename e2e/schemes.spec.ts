import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import yaml from 'js-yaml';
import { test, expect } from '@playwright/test';
import { buildSchemeIndex, schemeSlug } from '../src/lib/slugs';

const DATA = path.join(path.dirname(fileURLToPath(import.meta.url)), '..', 'data');
const readDir = <T>(dir: string): T[] =>
	fs
		.readdirSync(path.join(DATA, dir))
		.filter((f) => f.endsWith('.yaml'))
		.map((f) => yaml.load(fs.readFileSync(path.join(DATA, dir, f), 'utf8')) as T);

interface SourceScheme {
	name: string;
	slug?: string;
	versions: { parametersets: unknown[] }[];
}
const sigs = readDir<SourceScheme>('schemes');
const kems = readDir<{ name: string; slug?: string }>('kems');
const singleVersion = sigs.filter((s) => s.versions.length === 1).sort((a, b) => a.name.localeCompare(b.name))[0];

test.describe('Scheme pages', () => {
	test('header omits an assumption that repeats the category, and spaces the website', async ({ page }) => {
		await page.goto('/schemes/sqisign/');
		const meta = page.locator('h1 + p');
		await expect(meta).not.toContainText('Isogenies · Isogenies');
		await expect(meta.locator('a')).toHaveCount(1);
		expect(await meta.textContent()).toMatch(/\S\s+·\s+\S/);
	});

	test('KEM page states its status once, not as a Parameter sets subtitle', async ({ page }) => {
		await page.goto('/schemes/ml-kem/');
		await expect(page.locator('#sets-heading + p')).toHaveCount(0);
	});

	test('every signature and KEM has a prerendered page', async ({ request }) => {
		for (const ref of buildSchemeIndex(sigs, kems)) {
			const res = await request.get(`/schemes/${ref.slug}/`);
			expect(res.status(), ref.name).toBe(200);
		}
	});

	test('MAYO header: family, website, programme, latest version, flags', async ({ page }) => {
		await page.goto('/schemes/mayo/');
		await expect(page.locator('h1')).toHaveText('MAYO');
		const crumb = page.getByRole('navigation', { name: 'Breadcrumb' });
		await expect(crumb.getByRole('link', { name: 'Signatures' })).toHaveAttribute('href', '/signatures/');
		await expect(page.getByText('Multivariate · Multivariate Quadratic')).toBeVisible();
		await expect(page.getByRole('link', { name: /pqmayo\.org/ })).toHaveAttribute('href', 'https://pqmayo.org');
		await expect(page.getByText('On-ramp R3')).toBeVisible();
		await expect(page.getByText('Round 3 · 31 Aug 2026').first()).toBeVisible();
		await expect(page.getByText('None on current sets')).toBeVisible();
	});

	test('MAYO parameter sets and history slice', async ({ page }) => {
		await page.goto('/schemes/mayo/');
		const table = page.getByRole('table', { name: 'Parameter sets of MAYO' });
		// The scheme page lists every set: the 4 main sets plus the 30 additional ones of the round-3 spec.
		await expect(table.locator('tbody tr')).toHaveCount(34);
		const history = page.getByRole('region', { name: 'History' });
		await expect(history).toContainText('New version');
		await expect(history).toContainText('Wedge attack');
		await expect(history).not.toContainText('biased vinegar'); // SNOVA-only entry
	});

	test('a single-version scheme shows its sets and no comparison', async ({ page }) => {
		await page.goto(`/schemes/${schemeSlug(singleVersion)}/`);
		await expect(page.locator('h1')).toHaveText(singleVersion.name);
		await expect(
			page.getByRole('table', { name: `Parameter sets of ${singleVersion.name}` }).locator('tbody tr')
		).toHaveCount(singleVersion.versions[0].parametersets.length);
		await expect(page.getByRole('heading', { name: 'Compare versions' })).toHaveCount(0);
		await expect(page.getByText(/Only one version of .* is recorded/)).toBeVisible();
	});

	test('a KEM page has sets, programmes and no comparison', async ({ page }) => {
		await page.goto('/schemes/ml-kem/');
		await expect(page.locator('h1')).toHaveText('ML-KEM');
		await expect(
			page.getByRole('navigation', { name: 'Breadcrumb' }).getByRole('link', { name: 'KEMs' })
		).toBeVisible();
		const table = page.getByRole('table', { name: 'Parameter sets of ML-KEM' });
		await expect(table.getByRole('columnheader', { name: 'ct (B)', exact: true })).toBeVisible();
		await expect(table.getByRole('rowheader', { name: /ML-KEM-768/ })).toBeVisible();
		await expect(page.getByRole('heading', { name: 'Compare versions' })).toHaveCount(0);
	});

	test('no horizontal page scroll at 390px', async ({ page }) => {
		await page.setViewportSize({ width: 390, height: 800 });
		await page.goto('/schemes/mirith/');
		await expect(page.locator('h1')).toHaveText('MiRitH');
		expect(
			await page.evaluate(() => document.documentElement.scrollWidth <= document.documentElement.clientWidth)
		).toBe(true);
	});
});

test.describe('Compare versions (MAYO)', () => {
	const compareTable = (page: import('@playwright/test').Page) => page.getByRole('table', { name: /^Change from / });
	const setRow = (page: import('@playwright/test').Page, name: string) =>
		compareTable(page).locator('tbody tr', { has: page.getByRole('rowheader', { name: new RegExp(`^${name}\\b`) }) });

	test.beforeEach(async ({ page }) => {
		await page.goto('/schemes/mayo/');
		await page.waitForLoadState('networkidle');
	});

	test('defaults to the previous version against the latest and tells same-label versions apart', async ({ page }) => {
		await expect(page.getByRole('heading', { name: 'Compare versions' })).toBeVisible();
		await expect(page.getByLabel('From', { exact: true })).toHaveValue('2025-03-26');
		await expect(page.getByLabel('From', { exact: true }).locator('option:checked')).toHaveText(
			'Round 2 · 26 Mar 2025'
		);
		await expect(
			page.getByLabel('From', { exact: true }).locator('option', { hasText: 'Round 2 · 5 Feb 2025' })
		).toHaveCount(1);
		await expect(page.getByLabel('To', { exact: true })).toHaveValue('2026-08-31');
		const two = setRow(page, 'two');
		await expect(two).toContainText('4,912');
		await expect(two).toContainText('2,928');
		await expect(two).toContainText('▼ 40%');
		await expect(two).toContainText('▲ 28%');
		await expect(two).toContainText('Flag cleared');
		await expect(two.locator('.sr-only').filter({ hasText: 'Was Broken' })).toHaveCount(1);
		await expect(setRow(page, 'three')).toContainText('same');
	});

	test('swap reverses the comparison', async ({ page }) => {
		await page.getByRole('button', { name: 'Swap From and To' }).click();
		await expect(page.getByLabel('From', { exact: true })).toHaveValue('2026-08-31');
		const two = setRow(page, 'two');
		await expect(two).toContainText('▲ 68%');
		await expect(two).toContainText('Broken');
	});

	test('cycles mode shows cycle counts with the benchmark-source caveat', async ({ page }) => {
		await page.getByRole('button', { name: 'Cycles' }).click();
		await expect(page.getByRole('button', { name: 'Cycles' })).toHaveAttribute('aria-pressed', 'true');
		await expect(compareTable(page).getByRole('columnheader', { name: 'Sign (cycles)' })).toBeVisible();
		await expect(setRow(page, 'two')).toContainText('▼ 72%');
		const note = page.getByTestId('perf-note');
		await expect(note).toContainText('submission document (Table 3.1');
		await expect(note).toContainText('machines and implementations can differ');
	});

	test('cycles mode drops the duplicated source note when From equals To', async ({ page }) => {
		await page.getByRole('button', { name: 'Cycles' }).click();
		await expect(page.getByTestId('perf-note')).toBeVisible();
		await page.getByLabel('From', { exact: true }).selectOption('2026-08-31');
		await expect(page.getByTestId('perf-note')).toHaveCount(0);
	});

	test('picking an older From version re-matches the sets', async ({ page }) => {
		await page.getByLabel('From', { exact: true }).selectOption('2023-06-01');
		await expect(setRow(page, 'two')).toContainText('5,488');
		await expect(setRow(page, 'two')).not.toContainText('Flag cleared');
	});
});

test.describe('Compare versions layout', () => {
	test('no horizontal page scroll at 390px on MAYO compare, sizes and cycles', async ({ page }) => {
		await page.setViewportSize({ width: 390, height: 800 });
		await page.goto('/schemes/mayo/');
		await expect(page.getByRole('heading', { name: 'Compare versions' })).toBeVisible();
		const noScroll = () =>
			page.evaluate(() => document.documentElement.scrollWidth <= document.documentElement.clientWidth);
		expect(await noScroll()).toBe(true);
		await page.getByRole('button', { name: 'Cycles' }).click();
		expect(await noScroll()).toBe(true);
	});
});

test.describe('Links to scheme pages', () => {
	test('a signature table row links to its scheme page', async ({ page }) => {
		await page.goto('/signatures/');
		await page.waitForLoadState('networkidle');
		const link = page.getByRole('link', { name: 'MAYO', exact: true }).first();
		await expect(link).toHaveAttribute('href', '/schemes/mayo/');
		await link.click();
		await expect(page.locator('h1')).toHaveText('MAYO', { timeout: 15_000 });
	});

	test('a KEM table row links to its scheme page', async ({ page }) => {
		await page.goto('/kems/');
		await page.waitForLoadState('networkidle');
		const link = page.getByRole('link', { name: 'ML-KEM', exact: true }).first();
		await expect(link).toHaveAttribute('href', '/schemes/ml-kem/');
		await link.click();
		await expect(page.locator('h1')).toHaveText('ML-KEM', { timeout: 15_000 });
	});
});

test.describe('Chart points open scheme pages', () => {
	for (const route of ['/signatures/', '/kems/', '/programmes/iso/']) {
		test(`clicking a point on ${route} opens a scheme page`, async ({ page }) => {
			await page.goto(route);
			await page.waitForFunction(() => document.querySelector('.mark-symbol path') !== null, { timeout: 15_000 });
			await page.waitForLoadState('networkidle');
			// Any point will do: overlapping marks all carry an href.
			await page.locator('.mark-symbol path').first().click({ force: true });
			await expect(page).toHaveURL(/\/schemes\/[a-z0-9-]+\/$/, { timeout: 15_000 });
			await expect(page.locator('h1')).not.toBeEmpty();
		});
	}
});

test.describe('Dragging the chart does not navigate', () => {
	for (const route of ['/signatures/', '/kems/']) {
		test(`a pan released over a point on ${route} stays on the page`, async ({ page }) => {
			await page.goto(route);
			await page.waitForFunction(() => document.querySelector('.mark-symbol path') !== null, { timeout: 15_000 });
			await page.waitForLoadState('networkidle');
			const box = await page.locator('.mark-symbol path').first().boundingBox();
			expect(box).not.toBeNull();
			const x = box!.x + box!.width / 2;
			const y = box!.y + box!.height / 2;
			// The point pans with the cursor, so the release lands on it again.
			await page.mouse.move(x, y);
			await page.mouse.down();
			await page.mouse.move(x + 40, y, { steps: 8 });
			await page.mouse.up();
			await page.waitForTimeout(500);
			await expect(page).toHaveURL(new RegExp(`${route}(\\?.*)?$`));
		});
	}
});
