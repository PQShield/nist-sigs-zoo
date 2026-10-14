<script lang="ts" generics="R extends CompareRow, G, F extends CompareFilter, A extends string">
	import { onMount, untrack, type Snippet } from 'svelte';
	import { goto } from '$app/navigation';
	import { navigating } from '$app/state';
	import { base } from '$app/paths';
	import { urlSyncTarget } from '$lib/urlSync';
	import { encodeCommonFilters, filterSummary } from '$lib/compareFilters';
	import {
		decodeAxes,
		decodeCompareFilter,
		defaultAxes,
		defaultCompareFilter,
		encodeAxes,
		encodeCompareFilter,
		filterCompareRows,
		type CompareConfig,
		type CompareData,
		type CompareRow
	} from '$lib/compare';
	import { roundFromParam, roundParam, roundStore, type Round } from '$lib/roundStore';
	import type { BenchmarkEnv, CompareFilter } from '$lib/types';
	import BenchmarkEnvInfo from './BenchmarkEnvInfo.svelte';
	import ChartModeToggle from './ChartModeToggle.svelte';
	import CompareChart from './CompareChart.svelte';
	import CompareFilterPanel from './CompareFilterPanel.svelte';
	import CompareTable from './CompareTable.svelte';
	import FilterSidebar from './FilterSidebar.svelte';
	import RoundSelect from './RoundSelect.svelte';

	interface Props {
		config: CompareConfig<R, G, F, A>;
		/** The dataset for an on-ramp round; only called with 'latest' unless `rounds`. */
		load: (round: Round) => CompareData<R, G>;
		/** On-ramp round select under "NIST on-ramp", with the round in the URL (`r`). */
		rounds?: boolean;
		/** Axis selects and scale toggles instead of the Sizes / Speed toggle. */
		advanced?: boolean;
		hero?: Snippet;
		/** Shown between the chart and the table. */
		note?: Snippet;
		env?: BenchmarkEnv | null;
	}
	let { config, load, rounds = false, advanced = false, hero, note, env = null }: Props = $props();

	const cfg = untrack(() => config);
	const loadData = untrack(() => load);

	const defaultsFor = (d: CompareData<R, G>) =>
		defaultCompareFilter(
			cfg.filter,
			d.ranges,
			d.parameterSets.map((r) => r.scheme),
			d.groups
		);

	// Prerendered with the latest data; the URL (round, filters, axes) is applied in onMount.
	const initial = loadData('latest');
	let data = $state.raw(initial);
	let defaults = $state.raw(defaultsFor(initial));
	let filter = $state(defaultsFor(initial));
	let axes = $state(defaultAxes(cfg.chart));
	let urlReady = $state(false);

	const filteredRows = $derived(filterCompareRows(cfg.filter, data.parameterSets, filter));

	function applyRound(round: Round, params: URLSearchParams) {
		data = loadData(round);
		defaults = defaultsFor(data);
		filter = decodeCompareFilter(cfg.filter, defaults, params);
	}

	onMount(() => {
		const params = new URLSearchParams(window.location.search);
		if (advanced) axes = decodeAxes(cfg.chart, params);
		if (!rounds) {
			filter = decodeCompareFilter(cfg.filter, defaults, params);
			urlReady = true;
			return;
		}

		// The URL is the source of truth on load: pick the round first, then apply the filters to its data.
		let currentRound = roundFromParam(params.get('r'));
		roundStore.set(currentRound);
		applyRound(currentRound, params);
		urlReady = true;

		// subscribe() fires synchronously with the current value; the guard skips that call.
		return roundStore.subscribe((round) => {
			if (round === currentRound) return;
			currentRound = round;
			// Keep the data-independent filters (programmes, levels, parameter sets, status,
			// chart) and the sort; scheme and range filters reset to the new round's data.
			const { sortCol, sortDir } = filter;
			const carry = new URLSearchParams();
			encodeCommonFilters(filter, defaults, carry);
			applyRound(round, carry);
			filter = { ...filter, sortCol, sortDir };
		});
	});

	// Debounced filter → URL sync.
	$effect(() => {
		const p = encodeCompareFilter(cfg.filter, filter, defaults);
		if (advanced) encodeAxes(cfg.chart, axes, p);
		const r = rounds ? roundParam($roundStore) : null;
		if (r) p.set('r', r);
		const qs = p.toString();
		if (!urlReady) return;
		const timer = setTimeout(() => {
			const target = urlSyncTarget(
				qs,
				window.location.pathname,
				window.location.hash,
				window.location.search,
				navigating.to !== null
			);
			if (target === null) return;
			goto(target, { replaceState: true, keepFocus: true, noScroll: true });
		}, 300);
		return () => clearTimeout(timer);
	});

	const view = $derived(cfg.chart.views[filter.chart]);
	const selectClass =
		'rounded border border-pqs-ashgray bg-white px-2 py-1 text-sm text-pqs-midnight dark:border-pqs-steel dark:bg-pqs-midnight-mid dark:text-pqs-smoke focus:outline-none focus:ring-1 focus:ring-pqs-apricot';
	const scaleClass = (on: boolean) =>
		`px-2 py-1 text-xs font-heading transition-colors ${on ? 'bg-pqs-apricot text-pqs-midnight font-semibold' : 'bg-white text-pqs-steel hover:text-pqs-midnight dark:bg-pqs-midnight-mid dark:text-pqs-bluegray dark:hover:text-white'}`;
</script>

{#snippet onrampRound(enabled: boolean)}
	<RoundSelect disabled={!enabled} />
{/snippet}

{#snippet axisControl(label: string, axis: 'x' | 'y', scale: 'xs' | 'ys')}
	<div class="flex items-center gap-2">
		<span
			class="w-14 shrink-0 font-heading text-xs font-semibold tracking-wider text-pqs-steel uppercase dark:text-pqs-bluegray"
			>{label}</span
		>
		<select bind:value={axes[axis]} class="{selectClass} flex-1">
			{#each cfg.chart.axisOptions as field (field)}
				<option value={field}>{cfg.chart.axisTitles[field]}</option>
			{/each}
		</select>
		<div class="flex shrink-0 overflow-hidden rounded border border-pqs-ashgray dark:border-pqs-steel">
			<button onclick={() => (axes[scale] = 'log')} class={scaleClass(axes[scale] === 'log')}>log</button>
			<button onclick={() => (axes[scale] = 'linear')} class={scaleClass(axes[scale] === 'linear')}>linear</button>
		</div>
	</div>
{/snippet}

<div class="mx-auto max-w-[120rem] px-6 py-8">
	{#if advanced}
		<div class="mb-8 border-l-4 border-pqs-apricot pl-4">
			<div class="flex items-baseline gap-3">
				<h1 class="font-heading text-3xl font-bold text-pqs-midnight dark:text-white">Advanced Graph</h1>
				<a
					href="{base}/{cfg.path}/"
					class="text-sm text-pqs-steel transition-colors hover:text-pqs-apricot dark:text-pqs-bluegray dark:hover:text-pqs-apricot"
				>
					← Back
				</a>
			</div>
			<p class="mt-2 text-sm text-pqs-steel dark:text-pqs-bluegray">
				Customise axes to compare {cfg.noun} on any combination of size and performance dimensions.
			</p>
		</div>
	{:else}
		{@render hero?.()}
	{/if}

	<div class="flex flex-col gap-6 lg:flex-row lg:gap-8">
		<FilterSidebar summary={filterSummary(filter, filter.schemes.size)}>
			<CompareFilterPanel
				spec={cfg.filter}
				schemes={data.schemes}
				categories={data.categories}
				ranges={data.ranges}
				groups={data.groups}
				{defaults}
				bind:filter
				onramp={rounds ? onrampRound : undefined}
			/>
		</FilterSidebar>

		<!-- Main content -->
		<div class="min-w-0 flex-1 space-y-6">
			<!-- Scatter plot -->
			<section
				class="rounded border border-pqs-ashgray bg-white p-4 shadow-sm dark:border-pqs-steel dark:bg-pqs-midnight-mid"
			>
				{#if advanced}
					<h2 class="mb-4 font-heading text-base font-semibold text-pqs-steel dark:text-pqs-apricot">
						{cfg.chart.axisShort[axes.x]} vs. {cfg.chart.axisShort[axes.y]}
						<span class="font-normal text-pqs-steel dark:text-pqs-bluegray">({axes.xs}–{axes.ys} scale)</span>
					</h2>

					<div class="mb-4 grid grid-cols-1 gap-3 sm:grid-cols-2">
						{@render axisControl('X axis', 'x', 'xs')}
						{@render axisControl('Y axis', 'y', 'ys')}
					</div>

					<CompareChart
						config={cfg}
						rows={filteredRows}
						xField={axes.x}
						yField={axes.y}
						xScale={axes.xs}
						yScale={axes.ys}
					/>
				{:else}
					<div class="mb-3 flex flex-wrap items-center justify-between gap-3">
						<h2 class="font-heading text-base font-semibold text-pqs-steel dark:text-pqs-apricot">
							{view.title}
							<span class="font-normal text-pqs-bluegray">(log–log scale)</span>
						</h2>
						<ChartModeToggle mode={filter.chart} onchange={(chart) => (filter = { ...filter, chart })} />
					</div>
					<CompareChart config={cfg} rows={filteredRows} xField={view.x} yField={view.y} />
					<div class="mt-2 flex justify-end">
						<a
							href="{base}/{cfg.path}/advanced/"
							class="text-xs text-pqs-steel transition-colors hover:text-pqs-apricot dark:text-pqs-bluegray dark:hover:text-pqs-apricot"
						>
							Advanced graph →
						</a>
					</div>
				{/if}
			</section>

			{@render note?.()}

			<section>
				<CompareTable
					config={cfg}
					rows={filteredRows}
					sortCol={filter.sortCol}
					sortDir={filter.sortDir}
					onsort={(sortCol, sortDir) => (filter = { ...filter, sortCol, sortDir })}
				/>
			</section>

			{#if env}
				<BenchmarkEnvInfo {env} />
			{/if}
		</div>
	</div>
</div>
