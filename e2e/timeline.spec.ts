import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import yaml from 'js-yaml';
import { test, expect, type Page } from '@playwright/test';

// Expected counts come from the YAML (the source of truth), not from the rendered page.
const DATA = path.join(path.dirname(fileURLToPath(import.meta.url)), '..', 'data');
const load = <T>(file: string): T => yaml.load(fs.readFileSync(file, 'utf8')) as T;
const yamlFiles = (dir: string) => fs.readdirSync(path.join(DATA, dir)).filter((f) => f.endsWith('.yaml'));

interface Entry {
	type: string;
	schemes?: string[];
	programmes?: string[];
}
const HISTORY = load<Entry[]>(path.join(DATA, 'history.yaml'));

function programmesByScheme(): Map<string, Set<string>> {
	const index = new Map<string, Set<string>>();
	const add = (name: string, ids: string[]) => {
		const set = index.get(name) ?? new Set<string>();
		ids.forEach((id) => set.add(id));
		index.set(name, set);
	};
	for (const f of yamlFiles('schemes')) {
		const s = load<{ name: string; versions: { programmes?: Record<string, unknown> }[] }>(
			path.join(DATA, 'schemes', f)
		);
		add(
			s.name,
			s.versions.flatMap((v) => Object.keys(v.programmes ?? {}))
		);
	}
	for (const f of yamlFiles('kems')) {
		const k = load<{ name: string; programmes?: Record<string, unknown> }>(path.join(DATA, 'kems', f));
		add(k.name, Object.keys(k.programmes ?? {}));
	}
	return index;
}
const INDEX = programmesByScheme();
const inProgramme = (e: Entry, id: string) =>
	(e.programmes ?? []).includes(id) || (e.schemes ?? []).some((n) => INDEX.get(n)?.has(id));

const count = (page: Page) => page.getByRole('status').filter({ hasText: /^Showing/ });
const events = (page: Page) => page.getByRole('list', { name: 'Events' }).locator(':scope > li');

async function open(page: Page, url: string) {
	await page.goto(url);
	await page.waitForLoadState('networkidle');
}

test.describe('timeline', () => {
	test('lists every event by default, newest first', async ({ page }) => {
		await open(page, '/timeline/');
		await expect(count(page)).toHaveText(`Showing all ${HISTORY.length} events`);
		await expect(events(page)).toHaveCount(HISTORY.length);
		const dates = await page
			.getByRole('list', { name: 'Events' })
			.locator('time')
			.evaluateAll((els) => els.map((e) => e.getAttribute('datetime') ?? ''));
		expect(dates).toEqual([...dates].sort().reverse());
	});

	test('filters by event type and keeps the choice in the URL', async ({ page }) => {
		await open(page, '/timeline/');
		const types = page.getByRole('group', { name: 'Event type' });
		for (const name of ['Update', 'Milestone', 'Standardisation']) {
			const box = types.getByRole('checkbox', { name, exact: true });
			if (await box.count()) await box.uncheck();
		}
		const attacks = HISTORY.filter((e) => e.type === 'attack').length;
		await expect(count(page)).toHaveText(`Showing ${attacks} of ${HISTORY.length} events`);
		await expect(page).toHaveURL(/[?&]t=attack(&|$)/);

		await page.reload();
		await page.waitForLoadState('networkidle');
		await expect(count(page)).toHaveText(`Showing ${attacks} of ${HISTORY.length} events`);
		await expect(types.getByRole('checkbox', { name: 'Update', exact: true })).not.toBeChecked();
		await expect(types.getByRole('checkbox', { name: 'Attack', exact: true })).toBeChecked();
	});

	test('?p=kpqc shows only KpqC events, each with a KpqC chip', async ({ page }) => {
		const n = HISTORY.filter((e) => inProgramme(e, 'kpqc')).length;
		expect(n).toBeGreaterThan(0);
		await open(page, '/timeline/?p=kpqc');
		await expect(count(page)).toHaveText(`Showing ${n} of ${HISTORY.length} events`);
		await expect(events(page)).toHaveCount(n);
		for (let i = 0; i < n; i++) await expect(events(page).nth(i)).toContainText('KpqC');
		const programmes = page.getByRole('group', { name: 'Programme' });
		await expect(programmes.getByRole('checkbox', { name: 'KpqC', exact: true })).toBeChecked();
		await expect(programmes.getByRole('checkbox', { name: 'NIST on-ramp', exact: true })).not.toBeChecked();
	});

	test('an empty programme list shows the empty state and a reset', async ({ page }) => {
		await open(page, '/timeline/?p=');
		await expect(page.getByText('No events match these filters.')).toBeVisible();
		await page.getByRole('button', { name: 'Show all events' }).click();
		await expect(count(page)).toHaveText(`Showing all ${HISTORY.length} events`);
		await expect(page).not.toHaveURL(/[?&]p=/);
	});

	test('a scheme name links to its scheme page', async ({ page }) => {
		await open(page, '/timeline/');
		const link = page.getByRole('list', { name: 'Events' }).locator('a[href*="/schemes/"]').first();
		await expect(link).toBeVisible();
		const href = await link.getAttribute('href');
		await link.click();
		await page.waitForURL((u) => u.pathname === href);
		await expect(page.locator('h1')).toBeVisible();
	});

	test('programme chips link to the programme page', async ({ page }) => {
		await open(page, '/timeline/?p=kpqc');
		const chip = events(page).first().getByRole('link', { name: 'KpqC', exact: true });
		await expect(chip).toHaveAttribute('href', /\/programmes\/kpqc\/$/);
	});

	test('no horizontal page scroll at 390px', async ({ page }) => {
		await page.setViewportSize({ width: 390, height: 800 });
		await open(page, '/timeline/');
		const overflow = await page.evaluate(
			() => document.documentElement.scrollWidth - document.documentElement.clientWidth
		);
		expect(overflow).toBeLessThanOrEqual(0);
	});
});
