<script lang="ts">
	import { goto } from '$app/navigation';
	import { page } from '$app/stores';
	import { base } from '$app/paths';
	import { browser } from '$app/environment';
	import { get } from 'svelte/store';
	import { onMount } from 'svelte';
	import FilterPanel from '$lib/components/FilterPanel.svelte';
	import SchemeTable from '$lib/components/SchemeTable.svelte';
	import ScatterPlot from '$lib/components/ScatterPlot.svelte';
	import { processYamlSchemes } from '$lib/data';
	import { getFilterStore, buildUrlParams, createFilterStore } from '$lib/filterStore';
	import { allSchemeData } from '$lib/schemeData';
	import type { AxisField, ScaleType } from '$lib/types';
	import type { PageData } from './$types';

	let { data: _ }: { data: PageData } = $props();

	const _init = processYamlSchemes(allSchemeData, 'round-3', { useLatestVersion: true });
	let schemes = $state(_init.schemes);
	let categories = $state([...new Set(_init.schemes.map((s) => s.category))].sort());
	let ranges = $state(_init.ranges);

	createFilterStore(_init.parameterSets, _init.ranges, new URLSearchParams());

	const { applyUrl } = getFilterStore();

	let xField = $state<AxisField>('pk');
	let yField = $state<AxisField>('sig');
	let xScale = $state<ScaleType>('log');
	let yScale = $state<ScaleType>('log');

	const AXIS_OPTIONS: { value: AxisField; label: string }[] = [
		{ value: 'pk', label: 'Public key (bytes)' },
		{ value: 'sig', label: 'Signature (bytes)' },
		{ value: 'pkPlusSig', label: 'pk + sig (bytes)' },
		{ value: 'signingCycles', label: 'Signing (cycles)' },
		{ value: 'verificationCycles', label: 'Verification (cycles)' },
		{ value: 'signingUs', label: 'Signing (µs)' },
		{ value: 'verificationUs', label: 'Verification (µs)' },
	];

	const AXIS_SHORT: Record<AxisField, string> = {
		pk: 'pk size',
		sig: 'sig size',
		pkPlusSig: 'pk+sig',
		signingCycles: 'signing (cycles)',
		verificationCycles: 'verification (cycles)',
		signingUs: 'signing (µs)',
		verificationUs: 'verification (µs)',
	};

	const VALID_FIELDS = new Set<string>(['pk', 'sig', 'pkPlusSig', 'signingCycles', 'verificationCycles', 'signingUs', 'verificationUs']);

	let urlSyncTimer: ReturnType<typeof setTimeout> | null = null;

	function syncUrl() {
		if (!browser) return;
		if (urlSyncTimer) clearTimeout(urlSyncTimer);
		urlSyncTimer = setTimeout(() => {
			const { store, defaults } = getFilterStore();
			const state = get(store);
			const p = buildUrlParams(state, defaults);
			if (xField !== 'pk') p.set('x', xField);
			if (yField !== 'sig') p.set('y', yField);
			if (xScale !== 'log') p.set('xs', xScale);
			if (yScale !== 'log') p.set('ys', yScale);
			const qs = p.toString();
			const newUrl = qs ? `?${qs}` : $page.url.pathname;
			goto(newUrl, { replaceState: true, keepFocus: true, noScroll: true });
		}, 300);
	}

	onMount(() => {
		const params = new URLSearchParams(window.location.search);

		const x = params.get('x');
		const y = params.get('y');
		const xs = params.get('xs');
		const ys = params.get('ys');

		if (x && VALID_FIELDS.has(x)) xField = x as AxisField;
		if (y && VALID_FIELDS.has(y)) yField = y as AxisField;
		if (xs === 'log' || xs === 'linear') xScale = xs;
		if (ys === 'log' || ys === 'linear') yScale = ys;

		if (params.toString()) applyUrl(params);

		const { store } = getFilterStore();
		const unsubFilter = store.subscribe(syncUrl);

		return () => {
			unsubFilter();
			if (urlSyncTimer) clearTimeout(urlSyncTimer);
		};
	});

	$effect(() => {
		xField; yField; xScale; yScale;
		syncUrl();
	});

	const selectClass = 'rounded border border-pqs-ashgray bg-white px-2 py-1 text-sm text-pqs-midnight dark:border-pqs-steel dark:bg-pqs-midnight-mid dark:text-pqs-smoke focus:outline-none focus:ring-1 focus:ring-pqs-apricot';
</script>

<div class="mx-auto max-w-screen-2xl px-6 py-8">
	<div class="mb-8 border-l-4 border-pqs-apricot pl-4">
		<div class="flex items-baseline gap-3">
			<h1 class="font-heading text-3xl font-bold text-pqs-midnight dark:text-white">
				Advanced Graph
			</h1>
			<a href="{base}/" class="text-sm text-pqs-bluegray hover:text-pqs-apricot dark:text-pqs-steel dark:hover:text-pqs-apricot transition-colors">
				← Back
			</a>
		</div>
		<p class="mt-2 text-sm text-pqs-steel dark:text-pqs-bluegray">
			Customise axes to compare schemes on any combination of size and performance dimensions.
		</p>
	</div>

	<div class="flex gap-8">
		<!-- Sidebar filter panel -->
		<div class="hidden w-60 shrink-0 lg:block">
			<div class="sticky top-4">
				<FilterPanel {schemes} {categories} {ranges} />
			</div>
		</div>

		<!-- Main content -->
		<div class="min-w-0 flex-1 space-y-6">
			<!-- Mobile filter disclosure -->
			<details class="lg:hidden">
				<summary class="cursor-pointer rounded border border-pqs-bluegray bg-white px-3 py-2 font-heading text-sm font-semibold text-pqs-steel dark:border-pqs-steel dark:bg-pqs-midnight-mid dark:text-pqs-apricot">
					Filters ▼
				</summary>
				<div class="mt-2 rounded border border-pqs-ashgray bg-white p-3 dark:border-pqs-steel dark:bg-pqs-midnight-mid">
					<FilterPanel {schemes} {categories} {ranges} />
				</div>
			</details>

			<!-- Scatter plot -->
			<section class="rounded border border-pqs-ashgray bg-white p-4 shadow-sm dark:border-pqs-steel dark:bg-pqs-midnight-mid">
				<h2 class="mb-4 font-heading text-base font-semibold text-pqs-steel dark:text-pqs-apricot">
					{AXIS_SHORT[xField]} vs. {AXIS_SHORT[yField]}
					<span class="font-normal text-pqs-bluegray">({xScale}–{yScale} scale)</span>
				</h2>

				<!-- Axis controls -->
				<div class="mb-4 grid grid-cols-1 gap-3 sm:grid-cols-2">
					<div class="flex items-center gap-2">
						<span class="w-14 shrink-0 font-heading text-xs font-semibold uppercase tracking-wider text-pqs-bluegray dark:text-pqs-steel">X axis</span>
						<select bind:value={xField} class="{selectClass} flex-1">
							{#each AXIS_OPTIONS as opt}
								<option value={opt.value}>{opt.label}</option>
							{/each}
						</select>
						<div class="flex rounded border border-pqs-ashgray dark:border-pqs-steel overflow-hidden shrink-0">
							<button
								onclick={() => (xScale = 'log')}
								class="px-2 py-1 text-xs font-heading transition-colors {xScale === 'log' ? 'bg-pqs-apricot text-pqs-midnight font-semibold' : 'bg-white text-pqs-bluegray hover:text-pqs-midnight dark:bg-pqs-midnight-mid dark:text-pqs-steel dark:hover:text-white'}"
							>log</button>
							<button
								onclick={() => (xScale = 'linear')}
								class="px-2 py-1 text-xs font-heading transition-colors {xScale === 'linear' ? 'bg-pqs-apricot text-pqs-midnight font-semibold' : 'bg-white text-pqs-bluegray hover:text-pqs-midnight dark:bg-pqs-midnight-mid dark:text-pqs-steel dark:hover:text-white'}"
							>linear</button>
						</div>
					</div>
					<div class="flex items-center gap-2">
						<span class="w-14 shrink-0 font-heading text-xs font-semibold uppercase tracking-wider text-pqs-bluegray dark:text-pqs-steel">Y axis</span>
						<select bind:value={yField} class="{selectClass} flex-1">
							{#each AXIS_OPTIONS as opt}
								<option value={opt.value}>{opt.label}</option>
							{/each}
						</select>
						<div class="flex rounded border border-pqs-ashgray dark:border-pqs-steel overflow-hidden shrink-0">
							<button
								onclick={() => (yScale = 'log')}
								class="px-2 py-1 text-xs font-heading transition-colors {yScale === 'log' ? 'bg-pqs-apricot text-pqs-midnight font-semibold' : 'bg-white text-pqs-bluegray hover:text-pqs-midnight dark:bg-pqs-midnight-mid dark:text-pqs-steel dark:hover:text-white'}"
							>log</button>
							<button
								onclick={() => (yScale = 'linear')}
								class="px-2 py-1 text-xs font-heading transition-colors {yScale === 'linear' ? 'bg-pqs-apricot text-pqs-midnight font-semibold' : 'bg-white text-pqs-bluegray hover:text-pqs-midnight dark:bg-pqs-midnight-mid dark:text-pqs-steel dark:hover:text-white'}"
							>linear</button>
						</div>
					</div>
				</div>

				<ScatterPlot {xField} {yField} {xScale} {yScale} />
			</section>

			<!-- Scheme table -->
			<section>
				<SchemeTable />
			</section>
		</div>
	</div>
</div>
