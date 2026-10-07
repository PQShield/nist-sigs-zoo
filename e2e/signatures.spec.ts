import { test, expect } from '@playwright/test';

test.describe('Signatures page', () => {
	test('loads with correct heading', async ({ page }) => {
		await page.goto('/signatures/');
		await expect(page.locator('h1')).toHaveText('Post-Quantum Signature Schemes');
	});

	test('Vega scatter plot renders', async ({ page }) => {
		await page.goto('/signatures/');
		// Wait for the dynamic vega-embed import to complete and draw SVG
		await page.waitForFunction(
			() => [...document.querySelectorAll('svg')].some((s) => s.querySelector('g')),
			{ timeout: 15_000 }
		);
		const vegaSvg = await page.locator('svg').filter({ has: page.locator('g') }).first();
		await expect(vegaSvg).toBeVisible();
	});

	test('scatter plot section heading', async ({ page }) => {
		await page.goto('/signatures/');
		await expect(page.locator('section h2').first()).toContainText('pk size vs. sig size');
	});

	test('"Advanced graph →" link is present and points to /signatures/advanced/', async ({ page }) => {
		await page.goto('/signatures/');
		const link = page.getByRole('link', { name: 'Advanced graph →' });
		await expect(link).toBeVisible();
		await expect(link).toHaveAttribute('href', /advanced/);
	});

	test('navigating "Advanced graph →" lands on advanced page', async ({ page }) => {
		await page.goto('/signatures/');
		// Wait for hydration (dynamic chunks settle) so the click is an SPA
		// navigation, not a full reload that can time out under load.
		await page.waitForLoadState('networkidle');
		await page.getByRole('link', { name: 'Advanced graph →' }).click();
		await expect(page.locator('h1')).toHaveText('Advanced Graph', { timeout: 15_000 });
	});

	test('scheme table is present', async ({ page }) => {
		await page.goto('/signatures/');
		await expect(page.locator('table')).toBeVisible();
	});

	test('dataset selector offers Latest and rounds 3–1', async ({ page }) => {
		await page.goto('/signatures/');
		const select = page.getByLabel('Dataset');
		await expect(select).toBeVisible();
		await expect(select.locator('option')).toHaveText(['Latest specs', 'Round 3', 'Round 2', 'Round 1']);
	});

	test('choosing Round 2 updates the URL', async ({ page }) => {
		await page.goto('/signatures/');
		await page.waitForLoadState('networkidle');
		await page.getByLabel('Dataset').selectOption('round-2');
		await page.waitForURL(/[?&]r=2/, { timeout: 15_000 });
	});

	test.describe('shared links keep their filters', () => {
		async function schemeCells(page: import('@playwright/test').Page) {
			await page.waitForSelector('table tbody tr');
			await page.waitForTimeout(1000);
			const rows = page.locator('table tbody tr').filter({ hasNotText: 'No parameter sets match' });
			const n = await rows.count();
			const names: string[] = [];
			for (let i = 0; i < n; i++) names.push(((await rows.nth(i).locator('td a').first().innerText()) ?? '').trim());
			return names;
		}

		test('?r=2&s=MAYO,UOV&sort=sig shows only MAYO and UOV and keeps the URL', async ({ page }) => {
			await page.goto('/signatures/?r=2&s=MAYO,UOV&sort=sig');
			const names = await schemeCells(page);
			expect(names.length).toBeGreaterThan(0);
			for (const n of names) expect(['MAYO', 'UOV']).toContain(n);
			expect(names).toContain('MAYO');
			expect(names).toContain('UOV');
			expect(page.url()).toMatch(/s=MAYO/);
			expect(page.url()).toMatch(/[?&]r=2/);
			expect(page.url()).toMatch(/sort=sig/);
		});

		test('?s=MAYO shows only MAYO', async ({ page }) => {
			await page.goto('/signatures/?s=MAYO');
			const names = await schemeCells(page);
			expect(names.length).toBeGreaterThan(0);
			for (const n of names) expect(n).toBe('MAYO');
			expect(page.url()).toMatch(/s=MAYO/);
		});

		test('hash survives the URL sync', async ({ page }) => {
			await page.goto('/signatures/?s=MAYO#table');
			await schemeCells(page);
			expect(await page.evaluate(() => location.hash)).toBe('#table');
		});

		test('old home link with round and filters ends up filtered', async ({ page }) => {
			await page.goto('/?r=2&s=MAYO,UOV');
			await page.waitForURL(/\/signatures\//, { timeout: 15_000 });
			const names = await schemeCells(page);
			expect(names.length).toBeGreaterThan(0);
			for (const n of names) expect(['MAYO', 'UOV']).toContain(n);
		});
	});
});
