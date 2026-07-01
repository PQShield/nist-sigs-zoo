import { test, expect } from '@playwright/test';

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
		await expect(page.getByText('33 parameter sets')).toBeVisible();
		// The parameter-set cell may display a shortened name (scheme prefix
		// stripped), but always carries the full name as its title attribute.
		for (const name of ['ML-KEM-768', 'HQC-256', 'FrodoKEM-640-AES', 'mceliece6960119', 'BAT-769-1024', 'ntruhps2048509', 'sntrup761', 'X25519', 'P-256']) {
			await expect(page.locator(`td[title="${name}"]`)).toBeVisible();
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

		await panel.getByRole('button', { name: 'None' }).click();
		await expect(page.getByText('0 parameter sets')).toBeVisible();

		await panel.getByRole('button', { name: 'All' }).click();
		await expect(page.getByText('33 parameter sets')).toBeVisible();
	});

	test('level filter excludes pre-quantum schemes', async ({ page }) => {
		await page.goto('/kems/');
		const panel = page.locator('aside').first();
		// Unchecking the N/A (pre-quantum) level drops the four ECDH rows.
		await panel.getByLabel('N/A').uncheck();
		await expect(page.getByText('29 parameter sets')).toBeVisible();
		await expect(page.getByText('X25519', { exact: true })).toHaveCount(0);
	});

	test('persists filter state in the URL across reload', async ({ page }) => {
		await page.goto('/kems/');
		const panel = page.locator('aside').first();
		await panel.getByLabel('N/A').uncheck();
		// debounced URL sync (300ms); toHaveURL polls until it matches
		await expect(page).toHaveURL(/[?&]l=/);

		await page.reload();
		await expect(page.getByText('29 parameter sets')).toBeVisible();
		await expect(page.getByText('X25519', { exact: true })).toHaveCount(0);
	});
});
