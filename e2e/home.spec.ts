import { test, expect } from '@playwright/test';

test.describe('Home page', () => {
	test('lists every programme', async ({ page }) => {
		await page.goto('/');
		const list = page.getByRole('region', { name: 'Programmes' });
		for (const name of ['NIST', 'On-ramp', 'NGCC', 'KpqC', 'ISO']) {
			await expect(list.getByText(name, { exact: false }).first()).toBeVisible();
		}
	});

	test('shows KpqC signature picks', async ({ page }) => {
		await page.goto('/');
		await expect(page.getByRole('region', { name: 'Programmes' })).toContainText('AIMer, HAETAE');
	});

	test('links to the signatures and KEM comparisons', async ({ page }) => {
		await page.goto('/');
		await expect(page.getByRole('link', { name: 'Compare signatures' })).toHaveAttribute('href', /\/signatures\/$/);
		await expect(page.getByRole('link', { name: 'Compare KEMs' })).toHaveAttribute('href', /\/kems\/$/);
	});

	test('shows the three latest history entries', async ({ page }) => {
		await page.goto('/');
		await expect(page.getByRole('region', { name: 'Latest changes' }).getByRole('listitem')).toHaveCount(3);
	});

	test('plain visit does not redirect', async ({ page }) => {
		await page.goto('/');
		await page.waitForLoadState('networkidle');
		expect(new URL(page.url()).pathname).toBe('/');
	});
});
