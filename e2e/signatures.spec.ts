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

	test('"Advanced graph →" link is present and points to /signatures/advanced/', async ({ page }) => {
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

	test('on-ramp round selector offers Latest and rounds 3–1 under NIST on-ramp', async ({ page }) => {
		await page.goto('/signatures/');
		const select = page.getByLabel('On-ramp round');
		await expect(select).toBeVisible();
		await expect(select.locator('option')).toHaveText(['Latest specs', 'Round 3', 'Round 2', 'Round 1']);
		await expect(page.getByRole('group', { name: 'Programme' }).getByLabel('On-ramp round')).toBeVisible();
	});

	test('choosing Round 2 updates the URL', async ({ page }) => {
		await page.goto('/signatures/');
		await page.waitForLoadState('networkidle');
		await page.getByLabel('On-ramp round').selectOption('round-2');
		await page.waitForURL(/[?&]r=2/, { timeout: 15_000 });
	});

	test('changing the round keeps status and programme filters', async ({ page }) => {
		await page.goto('/signatures/?hw=1');
		await page.waitForLoadState('networkidle');
		await page.getByLabel('NIST standards').uncheck();
		await expect(page).toHaveURL(/[?&]p=/);
		await page.getByLabel('On-ramp round').selectOption('round-2');
		await page.waitForURL(/[?&]r=2/, { timeout: 15_000 });
		await expect(page).toHaveURL(/[?&]hw=1/);
		await expect(page).toHaveURL(/[?&]p=/);
		await expect(page.getByLabel('Hide warnings')).toBeChecked();
		await expect(page.getByLabel('NIST standards')).not.toBeChecked();
	});

	test('the round select is disabled while NIST on-ramp is unticked', async ({ page }) => {
		await page.goto('/signatures/');
		await page.waitForLoadState('networkidle');
		await page.getByLabel('NIST on-ramp').uncheck();
		await expect(page.getByLabel('On-ramp round')).toBeDisabled();
	});

	test('table has a Programmes column with membership chips', async ({ page }) => {
		await page.goto('/signatures/');
		await expect(page.getByRole('columnheader', { name: 'Programmes' })).toBeVisible();
		await expect(page.getByRole('columnheader', { name: /^Status/ })).toHaveCount(0);
		await expect(page.locator('tbody')).toContainText('On-ramp R3');
		await expect(page.locator('tbody')).toContainText('FIPS 204');
	});

	test('a security badge expands a detail row under its parameter set', async ({ page }) => {
		await page.goto('/signatures/?ps=all');
		await page.waitForLoadState('networkidle');
		const badge = page.locator('tbody').getByRole('button', { name: /: show details$/ }).first();
		await expect(badge).toHaveAttribute('aria-expanded', 'false');
		const id = await badge.getAttribute('aria-controls');
		expect(id).toBeTruthy();
		const detail = page.locator(`[id="${id}"]`);
		await expect(detail).toHaveCount(1);
		await expect(detail).toBeHidden();
		const title = (await badge.getAttribute('aria-label'))!.replace(': show details', '');
		await badge.click();
		await expect(badge).toHaveAttribute('aria-expanded', 'true');
		await expect(detail).toContainText(title);
		await expect(detail).toBeVisible();
		expect(await detail.evaluate((el) => el.tagName)).toBe('TR');
		await badge.click();
		await expect(badge).toHaveAttribute('aria-expanded', 'false');
		await expect(detail).toBeHidden();
	});

	test('table legend lists implementation findings separately from broken', async ({ page }) => {
		await page.goto('/signatures/');
		const legend = page.getByText('Select an icon for details').locator('..');
		await expect(legend).toContainText('broken by design');
		await expect(legend).toContainText('implementation findings');
		await expect(legend).toContainText('not yet benchmarked');
	});

	test('Speed toggle switches the chart to signing vs. verification time', async ({ page }) => {
		await page.goto('/signatures/');
		await page.waitForLoadState('networkidle');
		const axes = page.getByRole('group', { name: 'Chart axes' });
		await expect(axes.getByRole('button', { name: 'Sizes' })).toHaveAttribute('aria-pressed', 'true');
		await axes.getByRole('button', { name: 'Speed' }).click();
		await expect(axes.getByRole('button', { name: 'Speed' })).toHaveAttribute('aria-pressed', 'true');
		await expect(page.locator('section h2').first()).toContainText('Signing vs. verification time');
		await expect(page).toHaveURL(/[?&]v=speed/);
		await page.reload();
		await expect(page.locator('section h2').first()).toContainText('Signing vs. verification time');
	});

	test('Speed view notes parameter sets left out because they are not benchmarked', async ({ page }) => {
		// EagleSign (on-ramp round 1) has two sets with sizes but no timings. ps=all keeps them from being hidden as
		// non-recommended; unbenchmarked rows pass the default cycle filters.
		await page.goto('/signatures/?r=1&v=speed&ps=all');
		await page.waitForFunction(() => [...document.querySelectorAll('svg')].some((s) => s.querySelector('g')), { timeout: 15_000 });
		await expect(page.getByRole('note').filter({ hasText: 'not yet benchmarked' })).toBeVisible();
	});

	test.describe('shared links keep their filters', () => {
		async function readSchemeCells(page: import('@playwright/test').Page) {
			const rows = page.locator('table tbody tr:not([hidden])').filter({ hasNotText: 'No parameter sets match' });
			const n = await rows.count();
			const names: string[] = [];
			for (let i = 0; i < n; i++) names.push(((await rows.nth(i).locator('td a').first().innerText()) ?? '').trim());
			return names;
		}

		// Wait until the client-side URL/filter sync has settled: two consecutive reads agree.
		async function schemeCells(page: import('@playwright/test').Page) {
			await page.waitForSelector('table tbody tr');
			await page.waitForLoadState('networkidle');
			let previous: string | null = null;
			await expect
				.poll(async () => {
					const current = JSON.stringify(await readSchemeCells(page));
					const stable = current === previous;
					previous = current;
					return stable;
				}, { intervals: [100, 200, 300], timeout: 10_000 })
				.toBe(true);
			return readSchemeCells(page);
		}

		test('?r=2&s=MAYO,UOV&sort=sig shows only MAYO and UOV and keeps the URL', async ({ page }) => {
			await page.goto('/signatures/?r=2&s=MAYO,UOV&sort=sig');
			const names = await schemeCells(page);
			expect(names.length).toBeGreaterThan(0);
			for (const n of names) expect(['MAYO', 'UOV']).toContain(n);
			expect(names).toContain('MAYO');
			expect(names).toContain('UOV');
			expect(page.url()).toMatch(/s=MAYO/);
			expect(page.url()).toMatch(/[?&]r=2/);
			expect(page.url()).toMatch(/sort=sig/);
		});

		test('?s=MAYO shows only MAYO', async ({ page }) => {
			await page.goto('/signatures/?s=MAYO');
			const names = await schemeCells(page);
			expect(names.length).toBeGreaterThan(0);
			for (const n of names) expect(n).toBe('MAYO');
			expect(page.url()).toMatch(/s=MAYO/);
		});

		test('hash survives the URL sync', async ({ page }) => {
			await page.goto('/signatures/?s=MAYO#table');
			await schemeCells(page);
			expect(await page.evaluate(() => location.hash)).toBe('#table');
		});

		test('old home link with round and filters ends up filtered', async ({ page }) => {
			await page.goto('/?r=2&s=MAYO,UOV');
			await page.waitForURL(/\/signatures\//, { timeout: 15_000 });
			const names = await schemeCells(page);
			expect(names.length).toBeGreaterThan(0);
			for (const n of names) expect(['MAYO', 'UOV']).toContain(n);
		});
	});
});

test.describe('Signatures filters', () => {
	test('programme filter narrows the table and survives a reload', async ({ page }) => {
		await page.goto('/signatures/');
		await page.waitForLoadState('networkidle');
		const panel = page.locator('aside').first();
		await panel.getByLabel('NIST standards').uncheck();
		await expect(page).toHaveURL(/[?&]p=/);
		await expect(page.locator('tbody')).not.toContainText('ML-DSA');
		await page.reload();
		await page.waitForLoadState('networkidle');
		// The URL filter is applied in onMount, after hydration. The unchecked box only
		// exists once it has been applied, so wait for that before reading the table.
		await expect(panel.getByLabel('NIST standards')).not.toBeChecked({ timeout: 15_000 });
		await expect(page.locator('tbody')).not.toContainText('ML-DSA', { timeout: 15_000 });
	});

	test('security levels come in separate NIST and NGCC groups', async ({ page }) => {
		await page.goto('/signatures/');
		const nist = page.getByRole('group', { name: 'NIST category' });
		const ngcc = page.getByRole('group', { name: 'NGCC target (classical bits)' });
		await expect(nist.getByLabel('Level 1')).toBeChecked();
		await expect(nist.getByLabel('Pre-quantum', { exact: true })).toBeChecked();
		await expect(ngcc.getByLabel('128-bit')).toBeChecked();
		await expect(ngcc.getByLabel('512-bit')).toBeChecked();
	});

	test('parameter-set toggle defaults to recommended and is kept in the URL', async ({ page }) => {
		await page.goto('/signatures/');
		await page.waitForLoadState('networkidle');
		await expect(page.getByLabel('Recommended (where designers name them)')).toBeChecked();
		await page.getByLabel('All parameter sets').check();
		await expect(page).toHaveURL(/[?&]ps=all/);
	});

	test('status filters are three separate checkboxes', async ({ page }) => {
		await page.goto('/signatures/');
		await page.waitForLoadState('networkidle');
		const status = page.getByRole('group', { name: 'Security status' });
		await expect(status.getByLabel('Hide broken by design')).not.toBeChecked();
		await expect(status.getByLabel('Hide schemes with implementation findings')).not.toBeChecked();
		await status.getByLabel('Hide warnings').check();
		await expect(page).toHaveURL(/[?&]hw=1/);
	});

	test('filters start collapsed on a phone and open from the Filters button', async ({ page }) => {
		await page.setViewportSize({ width: 390, height: 844 });
		await page.goto('/signatures/');
		await page.waitForLoadState('networkidle');
		const toggle = page.getByRole('button', { name: /^Filters/ });
		await expect(toggle).toHaveAttribute('aria-expanded', 'false');
		await expect(page.getByLabel('NIST standards')).toBeHidden();
		await toggle.click();
		await expect(toggle).toHaveAttribute('aria-expanded', 'true');
		await expect(page.getByLabel('NIST standards')).toBeVisible();
	});

	test('desktop shows the filter column without a toggle', async ({ page }) => {
		await page.goto('/signatures/');
		await expect(page.getByRole('button', { name: /^Filters/ })).toBeHidden();
		await expect(page.getByLabel('NIST standards')).toBeVisible();
		await expect(page.locator('aside')).toHaveCount(1);
	});
});

test.describe('Signatures chart legend', () => {
	async function waitForChart(page: import('@playwright/test').Page) {
		await page.waitForFunction(
			() => [...document.querySelectorAll('svg')].some((s) => s.querySelector('g')),
			{ timeout: 15_000 }
		);
	}

	test('sits beside the plot on desktop and below it on a phone', async ({ page }) => {
		await page.goto('/signatures/');
		await waitForChart(page);
		const legend = page.getByRole('group', { name: 'Legend' });
		await expect(legend).toContainText('Family');
		await expect(legend).toContainText('NIST category');
		const chart = page.locator('section svg').filter({ has: page.locator('g') }).first();
		const l = (await legend.boundingBox())!;
		const c = (await chart.boundingBox())!;
		expect(l.x).toBeGreaterThanOrEqual(c.x + c.width - 1);

		await page.setViewportSize({ width: 390, height: 844 });
		await expect
			.poll(async () => (await legend.boundingBox())!.y - ((await chart.boundingBox())!.y + (await chart.boundingBox())!.height))
			.toBeGreaterThanOrEqual(-1);
	});
});

test.describe('China NGCC candidates', () => {
	test.use({ colorScheme: 'light' });

	test('rows show NGCC targets in purple, never as NIST levels', async ({ page }) => {
		await page.goto('/signatures/');
		await page.waitForLoadState('networkidle');
		await expect(page.getByRole('group', { name: 'Programme' }).getByLabel('China NGCC')).toBeChecked();
		const row = page.locator('tbody tr').filter({ hasText: 'Aigis-Sig+' }).filter({ hasText: '128-bit' }).first();
		await expect(row).toBeVisible();
		const level = row.getByTitle('China NGCC target in classical bits; not a NIST level');
		await expect(level).toHaveText('128-bit');
		await expect(level).toHaveCSS('color', 'rgb(107, 33, 168)');
	});
});
