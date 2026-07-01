<script lang="ts">
	import { onMount } from 'svelte';
	import { goto } from '$app/navigation';
	import { base } from '$app/paths';
	import KemFilterPanel from '$lib/components/KemFilterPanel.svelte';
	import KemScatterPlot from '$lib/components/KemScatterPlot.svelte';
	import KemTable from '$lib/components/KemTable.svelte';
	import {
		applyKemUrlParams,
		buildKemUrlParams,
		computeKemRanges,
		defaultKemFilter,
		filterKemRows,
		processKemSchemes
	} from '$lib/kemData';
	import { allKemData } from '$lib/kemSchemeData';
	import type { KemAxisField, ScaleType } from '$lib/types';
	import type { PageData } from './$types';

	// load (+page.ts) drives prerendering; data is read from the module glob here
	// (matching the signatures advanced page) so the one-time filter init stays warning-free.
	let { data: _ }: { data: PageData } = $props();

	const { schemes, parameterSets } = processKemSchemes(allKemData);
	const ranges = computeKemRanges(parameterSets);
	const categories = [...new Set(schemes.map((s) => s.category))].sort();

	const defaults = defaultKemFilter(ranges, schemes.map((s) => s.scheme));
	let filter = $state(defaultKemFilter(ranges, schemes.map((s) => s.scheme)));

	const filteredRows = $derived(filterKemRows(parameterSets, filter));

	let xField = $state<KemAxisField>('pk');
	let yField = $state<KemAxisField>('ct');
	let xScale = $state<ScaleType>('log');
	let yScale = $state<ScaleType>('log');

	const AXIS_OPTIONS: { value: KemAxisField; label: string }[] = [
		{ value: 'pk', label: 'Public key (bytes)' },
		{ value: 'ct', label: 'Ciphertext (bytes)' },
		{ value: 'pkPlusCt', label: 'pk + ct (bytes)' },
		{ value: 'keygenCycles', label: 'Keygen (cycles)' },
		{ value: 'encapsCycles', label: 'Encapsulation (cycles)' },
		{ value: 'decapsCycles', label: 'Decapsulation (cycles)' },
		{ value: 'keygenUs', label: 'Keygen (µs)' },
		{ value: 'encapsUs', label: 'Encapsulation (µs)' },
		{ value: 'decapsUs', label: 'Decapsulation (µs)' }
	];

	const AXIS_SHORT: Record<KemAxisField, string> = {
		pk: 'pk size',
		ct: 'ct size',
		pkPlusCt: 'pk+ct',
		keygenCycles: 'keygen (cycles)',
		encapsCycles: 'encaps (cycles)',
		decapsCycles: 'decaps (cycles)',
		keygenUs: 'keygen (µs)',
		encapsUs: 'encaps (µs)',
		decapsUs: 'decaps (µs)'
	};

	const VALID_FIELDS = new Set<string>([
		'pk',
		'ct',
		'pkPlusCt',
		'keygenCycles',
		'encapsCycles',
		'decapsCycles',
		'keygenUs',
		'encapsUs',
		'decapsUs'
	]);

	// URL state: filter + axis params encoded as query params (mirrors the signatures advanced page).
	// Applied client-side in onMount (the page is prerendered); synced back, debounced.
	let urlReady = $state(false);

	onMount(() => {
		const params = new URLSearchParams(window.location.search);

		const x = params.get('x');
		const y = params.get('y');
		const xs = params.get('xs');
		const ys = params.get('ys');

		if (x && VALID_FIELDS.has(x)) xField = x as KemAxisField;
		if (y && VALID_FIELDS.has(y)) yField = y as KemAxisField;
		if (xs === 'log' || xs === 'linear') xScale = xs;
		if (ys === 'log' || ys === 'linear') yScale = ys;

		if (params.toString()) filter = applyKemUrlParams(defaults, params);
		urlReady = true;
	});

	$effect(() => {
		filter; xField; yField; xScale; yScale;
		if (!urlReady) return;
		const timer = setTimeout(() => {
			const p = buildKemUrlParams(filter, defaults);
			if (xField !== 'pk') p.set('x', xField);
			if (yField !== 'ct') p.set('y', yField);
			if (xScale !== 'log') p.set('xs', xScale);
			if (yScale !== 'log') p.set('ys', yScale);
			const qs = p.toString();
			goto(qs ? `?${qs}` : window.location.pathname, {
				replaceState: true,
				keepFocus: true,
				noScroll: true
			});
		}, 300);
		return () => clearTimeout(timer);
	});

	const selectClass = 'rounded border border-pqs-ashgray bg-white px-2 py-1 text-sm text-pqs-midnight dark:border-pqs-steel dark:bg-pqs-midnight-mid dark:text-pqs-smoke focus:outline-none focus:ring-1 focus:ring-pqs-apricot';
</script>

<div class="mx-auto max-w-screen-2xl px-6 py-8">
	<div class="mb-8 border-l-4 border-pqs-apricot pl-4">
		<div class="flex items-baseline gap-3">
			<h1 class="font-heading text-3xl font-bold text-pqs-midnight dark:text-white">
				Advanced Graph
			</h1>
			<a href="{base}/kems/" class="text-sm text-pqs-bluegray hover:text-pqs-apricot dark:text-pqs-steel dark:hover:text-pqs-apricot transition-colors">
				← Back
			</a>
		</div>
		<p class="mt-2 text-sm text-pqs-steel dark:text-pqs-bluegray">
			Customise axes to compare KEMs on any combination of size and performance dimensions.
		</p>
	</div>

	<div class="flex gap-8">
		<!-- Sidebar filter panel -->
		<div class="hidden w-60 shrink-0 lg:block">
			<div class="sticky top-4">
				<KemFilterPanel {schemes} {categories} {ranges} bind:filter />
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
					<KemFilterPanel {schemes} {categories} {ranges} bind:filter />
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

				<KemScatterPlot rows={filteredRows} {xField} {yField} {xScale} {yScale} />
			</section>

			<!-- Table -->
			<section>
				<KemTable rows={filteredRows} />
			</section>
		</div>
	</div>
</div>
