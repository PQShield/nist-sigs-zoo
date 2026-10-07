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
const singleVersion = sigs
	.filter((s) => s.versions.length === 1)
	.sort((a, b) => a.name.localeCompare(b.name))[0];

test.describe('Scheme pages', () => {
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
		await expect(page.getByText('On-ramp round 3')).toBeVisible();
		await expect(page.getByText('Round 3 · 31 Aug 2026').first()).toBeVisible();
		await expect(page.getByText('None on current sets')).toBeVisible();
	});

	test('MAYO parameter sets and history slice', async ({ page }) => {
		await page.goto('/schemes/mayo/');
		const table = page.getByRole('table', { name: 'Parameter sets of MAYO' });
		await expect(table.locator('tbody tr')).toHaveCount(4);
		const history = page.getByRole('region', { name: 'History' });
		await expect(history).toContainText('New version');
		await expect(history).toContainText('Wedge attack');
		await expect(history).not.toContainText('biased vinegar'); // SNOVA-only entry
	});

	test('a single-version scheme shows its sets and no comparison', async ({ page }) => {
		await page.goto(`/schemes/${schemeSlug(singleVersion)}/`);
		await expect(page.locator('h1')).toHaveText(singleVersion.name);
		await expect(page.getByRole('table', { name: `Parameter sets of ${singleVersion.name}` }).locator('tbody tr')).toHaveCount(
			singleVersion.versions[0].parametersets.length
		);
		await expect(page.getByRole('heading', { name: 'Compare versions' })).toHaveCount(0);
		await expect(page.getByText(/Only one version of .* is recorded/)).toBeVisible();
	});

	test('a KEM page has sets, programmes and no comparison', async ({ page }) => {
		await page.goto('/schemes/ml-kem/');
		await expect(page.locator('h1')).toHaveText('ML-KEM');
		await expect(page.getByRole('navigation', { name: 'Breadcrumb' }).getByRole('link', { name: 'KEMs' })).toBeVisible();
		const table = page.getByRole('table', { name: 'Parameter sets of ML-KEM' });
		await expect(table.getByRole('columnheader', { name: 'ct (B)', exact: true })).toBeVisible();
		await expect(table.getByRole('rowheader', { name: /ML-KEM-768/ })).toBeVisible();
		await expect(page.getByRole('heading', { name: 'Compare versions' })).toHaveCount(0);
	});

	test('no horizontal page scroll at 390px', async ({ page }) => {
		await page.setViewportSize({ width: 390, height: 800 });
		await page.goto('/schemes/mirith/');
		await expect(page.locator('h1')).toHaveText('MiRitH');
		expect(await page.evaluate(() => document.documentElement.scrollWidth <= document.documentElement.clientWidth)).toBe(true);
	});
});
