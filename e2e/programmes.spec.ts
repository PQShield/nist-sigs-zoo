import { test, expect } from '@playwright/test';

const IDS = ['nist-pqc', 'nist-onramp', 'china-ngcc', 'kpqc', 'iso'];

test.describe('Programme pages', () => {
	for (const id of IDS) {
		test(`${id} page has a heading, breadcrumb and facts`, async ({ page }) => {
			await page.goto(`/programmes/${id}/`);
			await expect(page.locator('h1')).toBeVisible();
			const crumbs = page.getByRole('navigation', { name: 'Breadcrumb' });
			await expect(crumbs.getByRole('link', { name: 'Programmes' })).toHaveAttribute('href', /\/programmes\/$/);
			await expect(crumbs.locator('[aria-current="page"]')).toBeVisible();
			await expect(page.locator('main dl dt').first()).toHaveText('Organiser');
			await expect(page.getByRole('region', { name: 'Timeline' })).toBeVisible();
		});
	}

	test('China NGCC page shows its timeline, security targets and findings box', async ({ page }) => {
		await page.goto('/programmes/china-ngcc/');
		await expect(page.locator('h1')).toHaveText('Next-generation commercial cryptographic algorithms (NGCC)');
		const timeline = page.getByRole('region', { name: 'Timeline' });
		await expect(timeline).toContainText('Submissions closed');
		await expect(timeline).toContainText('30 Jun 2026');
		await expect(timeline.locator('[aria-current="step"]')).toContainText('Round 1');
		const levels = page.getByRole('region', { name: 'Security levels' });
		await expect(levels.locator('tbody th')).toHaveText(['128-bit', '256-bit', '384-bit', '512-bit']);
		const findings = page.getByRole('region', { name: 'Published findings' });
		await expect(findings).toContainText('Signatures');
		await expect(findings).toContainText('KEMs');
		await expect(findings.getByRole('link', { name: 'ngcc.dev' })).toHaveAttribute('href', 'https://ngcc.dev/');
	});

	test('NIST pages show the NIST category table and no findings box', async ({ page }) => {
		await page.goto('/programmes/nist-onramp/');
		await expect(page.getByRole('region', { name: 'Security levels' }).locator('tbody tr')).toHaveCount(5);
		await expect(page.getByRole('region', { name: 'Published findings' })).toHaveCount(0);
	});
});
