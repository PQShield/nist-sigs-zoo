import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import yaml from 'js-yaml';
import { test, expect } from '@playwright/test';

// NGCC data arrives with plan 3; expectations are read from data/ so these tests pass with or without it.
const SCHEMES_DIR = path.join(path.dirname(fileURLToPath(import.meta.url)), '..', 'data', 'schemes');

function signatureMemberCount(programme: string): number {
	return fs
		.readdirSync(SCHEMES_DIR)
		.filter((f) => f.endsWith('.yaml'))
		.filter((f) => {
			const s = yaml.load(fs.readFileSync(path.join(SCHEMES_DIR, f), 'utf8')) as {
				versions: { programmes?: Record<string, unknown> }[];
			};
			return s.versions.some((v) => v.programmes !== undefined && programme in v.programmes);
		}).length;
}

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

	test('on-ramp stage toggle shows each round’s entrants', async ({ page }) => {
		await page.goto('/programmes/nist-onramp/');
		await page.waitForLoadState('networkidle');
		const candidates = page.getByRole('region', { name: 'Candidates' });
		const stages = candidates.getByRole('group', { name: 'Stage' });
		await expect(stages.getByRole('button', { name: 'All, latest specs' })).toHaveAttribute('aria-pressed', 'true');

		await stages.getByRole('button', { name: 'Round 1' }).click();
		await expect(stages.getByRole('button', { name: 'Round 1' })).toHaveAttribute('aria-pressed', 'true');
		await expect(candidates.locator('tbody')).toContainText('3WISE');
		await expect(candidates.getByRole('link', { name: /signature comparison/ })).toHaveAttribute('href', /\/signatures\/\?p=nist-onramp&r=1$/);

		await stages.getByRole('button', { name: 'Round 2' }).click();
		await expect(candidates.locator('tbody')).not.toContainText('3WISE');
		await expect(candidates.locator('tbody')).toContainText('MAYO');
	});

	test('NIST PQC primitive toggle switches between signatures and KEMs', async ({ page }) => {
		await page.goto('/programmes/nist-pqc/');
		await page.waitForLoadState('networkidle');
		const candidates = page.getByRole('region', { name: 'Standards and candidates' });
		const kinds = candidates.getByRole('group', { name: 'Primitive' });
		await expect(candidates.locator('tbody')).toContainText('ML-DSA');
		await kinds.getByRole('button', { name: /^KEMs/ }).click();
		await expect(kinds.getByRole('button', { name: /^KEMs/ })).toHaveAttribute('aria-pressed', 'true');
		await expect(candidates.locator('tbody')).toContainText('ML-KEM');
	});

	test('ISO page lists its KEMs without a primitive toggle', async ({ page }) => {
		await page.goto('/programmes/iso/');
		const candidates = page.getByRole('region', { name: 'Schemes in the amendment' });
		await expect(candidates.getByRole('group', { name: 'Primitive' })).toHaveCount(0);
		await expect(candidates.locator('tbody')).toContainText('FrodoKEM');
	});

	test('China NGCC candidates render with or without imported data', async ({ page }) => {
		await page.goto('/programmes/china-ngcc/');
		const candidates = page.getByRole('region', { name: 'Round 1 candidates' });
		if (signatureMemberCount('china-ngcc') === 0) {
			await expect(candidates).toContainText('No signatures from this programme are in our data yet.');
			await expect(candidates.locator('table')).toHaveCount(0);
		} else {
			await expect(candidates.locator('tbody tr').first()).toBeVisible();
			await expect(candidates.getByRole('link', { name: /signature comparison/ })).toHaveAttribute('href', /\/signatures\/\?p=china-ngcc/);
		}
	});

	test('leaving a programme page restores the full signature comparison', async ({ page }) => {
		await page.goto('/programmes/kpqc/');
		await page.waitForLoadState('networkidle');
		await expect(page.getByRole('region', { name: 'Candidates' }).locator('tbody')).not.toContainText('ML-DSA');
		await page.getByRole('banner').getByRole('link', { name: 'Signatures', exact: true }).click();
		await expect(page.locator('h1')).toHaveText('Post-Quantum Signature Schemes', { timeout: 15_000 });
		await expect(page.locator('tbody')).toContainText('ML-DSA');
	});

	test('switching stage then leaving for Signatures logs no errors', async ({ page }) => {
		const errors: string[] = [];
		page.on('pageerror', (e) => errors.push(String(e)));
		page.on('console', (m) => {
			if (m.type() === 'error') errors.push(m.text());
		});
		await page.goto('/programmes/nist-onramp/');
		await page.waitForLoadState('networkidle');
		const stages = page.getByRole('region', { name: 'Candidates' }).getByRole('group', { name: 'Stage' });
		await stages.getByRole('button', { name: 'Round 1' }).click();
		await expect(stages.getByRole('button', { name: 'Round 1' })).toHaveAttribute('aria-pressed', 'true');
		await page.getByRole('banner').getByRole('link', { name: 'Signatures', exact: true }).click();
		await expect(page.locator('h1')).toHaveText('Post-Quantum Signature Schemes', { timeout: 15_000 });
		await expect(page.locator('tbody')).toContainText('ML-DSA');
		await page.waitForLoadState('networkidle');
		expect(errors).toEqual([]);
	});

	test('index links to every programme page', async ({ page }) => {
		await page.goto('/programmes/');
		await expect(page.locator('h1')).toHaveText('Programmes');
		for (const id of IDS) {
			await expect(page.locator(`main a[href$="/programmes/${id}/"]`)).toHaveCount(1);
		}
	});

	test('nav links to the programme index and marks it current on programme pages', async ({ page }) => {
		await page.goto('/programmes/kpqc/');
		const link = page.getByRole('banner').getByRole('link', { name: 'Programmes', exact: true });
		await expect(link).toHaveAttribute('href', /\/programmes\/$/);
		await expect(link).toHaveAttribute('aria-current', 'page');
	});

	test('home programme table links to programme pages', async ({ page }) => {
		await page.goto('/');
		const region = page.getByRole('region', { name: 'Programmes' });
		for (const id of IDS) {
			await expect(region.locator(`a[href$="/programmes/${id}/"]`)).toHaveCount(1);
		}
	});
});
