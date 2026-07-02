import { test, expect, type Page } from '@playwright/test';

// Real data rows carry a title attribute on the Parameter Set cell (4th column);
// the "No KEMs to display" fallback row has a single colspan cell and never
// matches, so these locators naturally exclude it without any literal counts.
const paramSetCellSelector = 'tbody tr td:nth-child(4)[title]';
const levelCellSelector = 'tbody tr td:nth-child(5)';

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
		await page.goto('/');
		// Wait for hydration so the click is an SPA navigation, not a full reload
		// that can time out under load.
		await page.waitForLoadState('networkidle');
		await page.getByRole('link', { name: 'KEMs' }).first().click();
		await expect(page.locator('h1')).toHaveText('Key Encapsulation Mechanisms', {
			timeout: 15_000
		});
	});

	test('has a Signatures link back to the home page', async ({ page }) => {
		await page.goto('/kems/');
		await page.waitForLoadState('networkidle');
		await page.getByRole('link', { name: 'Signatures' }).first().click();
		await expect(page.locator('h1')).toHaveText('Post-Quantum Signature Schemes', {
			timeout: 15_000
		});
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
		expect(rowCount).toBeGreaterThan(0);
		expect(await paramSetCount(page)).toBe(rowCount);

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

		const initialCount = await paramSetCount(page);
		expect(initialCount).toBeGreaterThan(0);

		await panel.getByRole('button', { name: 'None' }).click();
		await expect(page.getByText('0 parameter sets')).toBeVisible();

		await panel.getByRole('button', { name: 'All' }).click();
		expect(await paramSetCount(page)).toBe(initialCount);
	});

	test('level filter excludes pre-quantum schemes', async ({ page }) => {
		await page.goto('/kems/');
		const panel = page.locator('aside').first();

		const totalBefore = await paramSetCount(page);
		const naCountBefore = (await levelTexts(page)).filter((l) => l === 'N/A').length;
		// Sanity check: the assertions below are meaningless if there's nothing to exclude.
		expect(naCountBefore).toBeGreaterThan(0);

		await panel.getByLabel('N/A').uncheck();

		expect(await paramSetCount(page)).toBe(totalBefore - naCountBefore);
		expect(await levelTexts(page)).not.toContain('N/A');
	});

	test('persists filter state in the URL across reload', async ({ page }) => {
		await page.goto('/kems/');
		const panel = page.locator('aside').first();

		const totalBefore = await paramSetCount(page);
		const naCountBefore = (await levelTexts(page)).filter((l) => l === 'N/A').length;
		expect(naCountBefore).toBeGreaterThan(0);

		await panel.getByLabel('N/A').uncheck();
		// debounced URL sync (300ms); toHaveURL polls until it matches
		await expect(page).toHaveURL(/[?&]l=/);

		await page.reload();
		expect(await paramSetCount(page)).toBe(totalBefore - naCountBefore);
		expect(await levelTexts(page)).not.toContain('N/A');
	});
});
