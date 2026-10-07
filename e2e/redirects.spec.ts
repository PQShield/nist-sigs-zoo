import { test, expect } from '@playwright/test';

test.describe('Legacy URLs', () => {
	test('old home link with filters lands on /signatures/ with the same query', async ({ page }) => {
		await page.goto('/?r=2&sort=sig');
		await page.waitForURL(/\/signatures\/\?r=2&sort=sig$/, { timeout: 15_000 });
		await expect(page.locator('h1')).toHaveText('Post-Quantum Signature Schemes');
	});

	test('/advanced/ keeps its axis params', async ({ page }) => {
		await page.goto('/advanced/?x=pkPlusSig&ys=linear');
		await page.waitForURL(/\/signatures\/advanced\/\?x=pkPlusSig&ys=linear$/, { timeout: 15_000 });
		await expect(page.locator('h1')).toHaveText('Advanced Graph');
	});

	test('/advanced/ keeps its params even when the JS bundle loads slowly', async ({ page }) => {
		await page.route('**/_app/**', async (route) => {
			await new Promise((r) => setTimeout(r, 1500));
			await route.continue();
		});
		await page.goto('/advanced/?x=pkPlusSig&ys=linear');
		await page.waitForURL(/\/signatures\/advanced\/\?x=pkPlusSig&ys=linear$/, { timeout: 15_000 });
		expect(page.url()).toMatch(/\/signatures\/advanced\/\?x=pkPlusSig&ys=linear$/);
	});

	test('/history/ moves to /timeline/', async ({ page }) => {
		await page.goto('/history/');
		await page.waitForURL(/\/timeline\/$/, { timeout: 15_000 });
		await expect(page.locator('h1')).toHaveText('History');
	});

	test('redirect stubs declare the canonical URL for search engines', async ({ request }) => {
		const html = await (await request.get('/advanced/')).text();
		expect(html).toContain('rel="canonical"');
		expect(html).toContain('/signatures/advanced/');
	});
});
