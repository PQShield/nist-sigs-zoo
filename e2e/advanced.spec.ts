import { test, expect } from '@playwright/test';

test.describe('Advanced page', () => {
	test('loads with correct heading', async ({ page }) => {
		await page.goto('/advanced/');
		await expect(page.locator('h1')).toHaveText('Advanced Graph');
	});

	test('"← Back" link returns to main page', async ({ page }) => {
		await page.goto('/advanced/');
		const back = page.getByRole('link', { name: '← Back' });
		await expect(back).toBeVisible();
		await back.click();
		await page.waitForLoadState('networkidle');
		await expect(page.locator('h1')).toHaveText('Post-Quantum Signature Schemes');
	});

	test('two axis select dropdowns present', async ({ page }) => {
		await page.goto('/advanced/');
		const selects = page.locator('select');
		await expect(selects).toHaveCount(2);
	});

	test('each axis has log and linear scale buttons', async ({ page }) => {
		await page.goto('/advanced/');
		await expect(page.getByRole('button', { name: 'log' })).toHaveCount(2);
		await expect(page.getByRole('button', { name: 'linear' })).toHaveCount(2);
	});

	test('Vega scatter plot renders', async ({ page }) => {
		await page.goto('/advanced/');
		await page.waitForFunction(
			() => [...document.querySelectorAll('svg')].some((s) => s.querySelector('g')),
			{ timeout: 15_000 }
		);
		const vegaSvg = page.locator('svg').filter({ has: page.locator('g') }).first();
		await expect(vegaSvg).toBeVisible();
	});

	test('default plot heading is pk size vs. sig size (log–log)', async ({ page }) => {
		await page.goto('/advanced/');
		await expect(page.locator('section h2')).toContainText('pk size vs. sig size');
		await expect(page.locator('section h2')).toContainText('log–log');
	});

	test('changing X axis updates heading', async ({ page }) => {
		await page.goto('/advanced/');
		await page.locator('select').first().selectOption('pkPlusSig');
		await expect(page.locator('section h2')).toContainText('pk+sig vs. sig size');
	});

	test('changing Y axis updates heading', async ({ page }) => {
		await page.goto('/advanced/');
		await page.locator('select').nth(1).selectOption('signingCycles');
		await expect(page.locator('section h2')).toContainText('pk size vs. signing time');
	});

	test('toggling X scale to linear updates heading', async ({ page }) => {
		await page.goto('/advanced/');
		await page.getByRole('button', { name: 'linear' }).first().click();
		await expect(page.locator('section h2')).toContainText('linear–log');
	});

	test('toggling Y scale to linear updates heading', async ({ page }) => {
		await page.goto('/advanced/');
		await page.getByRole('button', { name: 'linear' }).nth(1).click();
		await expect(page.locator('section h2')).toContainText('log–linear');
	});

	test('non-default axis is encoded in URL', async ({ page }) => {
		await page.goto('/advanced/');
		await page.locator('select').first().selectOption('sig');
		await page.waitForTimeout(400); // debounce
		expect(new URL(page.url()).searchParams.get('x')).toBe('sig');
	});

	test('default axis values are NOT encoded in URL', async ({ page }) => {
		await page.goto('/advanced/');
		await page.waitForTimeout(400);
		const params = new URL(page.url()).searchParams;
		expect(params.has('x')).toBe(false);
		expect(params.has('y')).toBe(false);
		expect(params.has('xs')).toBe(false);
		expect(params.has('ys')).toBe(false);
	});

	test('URL state is restored on page load', async ({ page }) => {
		await page.goto('/advanced/?x=pkPlusSig&y=signingCycles&ys=linear');
		await expect(page.locator('section h2')).toContainText('pk+sig vs. signing time');
		await expect(page.locator('section h2')).toContainText('log–linear');
		// Select values reflect URL state
		await expect(page.locator('select').first()).toHaveValue('pkPlusSig');
		await expect(page.locator('select').nth(1)).toHaveValue('signingCycles');
	});

	test('Y scale linear button is highlighted after toggle', async ({ page }) => {
		await page.goto('/advanced/');
		const yLinear = page.getByRole('button', { name: 'linear' }).nth(1);
		await yLinear.click();
		// Active button gets bg-pqs-apricot class
		await expect(yLinear).toHaveClass(/bg-pqs-apricot/);
	});

	test('scheme table is present', async ({ page }) => {
		await page.goto('/advanced/');
		await expect(page.locator('table')).toBeVisible();
	});

	test('filter panel is present', async ({ page }) => {
		await page.goto('/advanced/');
		// Filter panel has checkboxes for scheme selection
		await expect(page.locator('input[type="checkbox"]').first()).toBeVisible();
	});
});
