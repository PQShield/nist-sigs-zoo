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

	test('"Advanced graph →" link is present and points to /advanced/', async ({ page }) => {
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
});
