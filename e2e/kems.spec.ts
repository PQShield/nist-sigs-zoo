import { test, expect } from '@playwright/test';

test.describe('KEM page', () => {
	test('loads with correct heading', async ({ page }) => {
		await page.goto('/kems/');
		await expect(page.locator('h1')).toHaveText('Post-Quantum KEMs');
	});

	test('Vega scatter plot renders', async ({ page }) => {
		await page.goto('/kems/');
		// Wait for the dynamic vega-embed import to complete and draw SVG
		await page.waitForFunction(
			() => [...document.querySelectorAll('svg')].some((s) => s.querySelector('g')),
			{ timeout: 15_000 }
		);
		const vegaSvg = page.locator('svg').filter({ has: page.locator('g') }).first();
		await expect(vegaSvg).toBeVisible();
	});

	test('scatter plot section heading', async ({ page }) => {
		await page.goto('/kems/');
		await expect(page.locator('section h2').first()).toContainText('pk size vs. ct size');
	});

	test('scheme table is present with KEM columns', async ({ page }) => {
		await page.goto('/kems/');
		await expect(page.locator('table')).toBeVisible();
		await expect(page.getByRole('columnheader', { name: 'ct (B)', exact: true })).toBeVisible();
		await expect(page.getByRole('columnheader', { name: 'pk+ct (B)', exact: true })).toBeVisible();
		await expect(page.getByRole('columnheader', { name: 'Encaps', exact: true })).toBeVisible();
		await expect(page.getByRole('columnheader', { name: 'Decaps', exact: true })).toBeVisible();
	});

	test('table includes the initial KEM schemes', async ({ page }) => {
		await page.goto('/kems/');
		const table = page.locator('table');
		for (const scheme of ['ML-KEM', 'HQC', 'ECDH']) {
			await expect(table.getByRole('link', { name: scheme }).first()).toBeVisible();
		}
	});

	test('filter panel is present', async ({ page }) => {
		await page.goto('/kems/');
		await expect(page.locator('input[type="checkbox"]').first()).toBeVisible();
	});

	test('sorting by ct encodes sort param in URL', async ({ page }) => {
		await page.goto('/kems/');
		await page.getByRole('columnheader', { name: 'ct (B)', exact: true }).click();
		await page.waitForTimeout(400); // debounce
		expect(new URL(page.url()).searchParams.get('sort')).toBe('ct');
	});

	test('default state has no query params', async ({ page }) => {
		await page.goto('/kems/');
		await page.waitForTimeout(400);
		const params = new URL(page.url()).searchParams;
		expect(params.has('sort')).toBe(false);
		expect(params.has('s')).toBe(false);
		expect(params.has('l')).toBe(false);
	});

	test('filter state restores from URL', async ({ page }) => {
		await page.goto('/kems/?s=ML-KEM&sort=pk&dir=desc');
		const table = page.locator('table');
		await expect(table.getByRole('link', { name: 'ML-KEM' }).first()).toBeVisible();
		await expect(table.getByRole('link', { name: 'HQC' })).toHaveCount(0);
		await expect(table.getByRole('link', { name: 'ECDH' })).toHaveCount(0);
	});

	test('"← Signatures" link returns to the signatures page', async ({ page }) => {
		await page.goto('/kems/');
		const back = page.getByRole('link', { name: '← Signatures' });
		await expect(back).toBeVisible();
		await back.click();
		await page.waitForLoadState('networkidle');
		await expect(page.locator('h1')).toHaveText('Post-Quantum Signature Schemes');
	});

	test('KEMs nav link is present and navigates from home', async ({ page }) => {
		await page.goto('/');
		await Promise.all([
			page.waitForURL(/\/kems/),
			page.getByRole('link', { name: 'KEMs', exact: true }).first().click(),
		]);
		await expect(page.locator('h1')).toHaveText('Post-Quantum KEMs');
	});
});
