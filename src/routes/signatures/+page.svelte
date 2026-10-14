<script lang="ts">
	import { SITE_NAME } from '$lib/constants';
	import { goto } from '$app/navigation';
	import { navigating } from '$app/state';
	import { urlSyncTarget } from '$lib/urlSync';
	import { page } from '$app/stores';
	import { base } from '$app/paths';
	import { onMount } from 'svelte';
	import { get } from 'svelte/store';
	import BenchmarkEnvInfo from '$lib/components/BenchmarkEnvInfo.svelte';
	import ChartModeToggle from '$lib/components/ChartModeToggle.svelte';
	import RoundSelect from '$lib/components/RoundSelect.svelte';
	import FilterSidebar from '$lib/components/FilterSidebar.svelte';
	import { encodeCommonFilters, filterSummary, presentProgrammeGroups } from '$lib/compareFilters';
	import FilterPanel from '$lib/components/FilterPanel.svelte';
	import CompareTable from '$lib/components/CompareTable.svelte';
	import { SIGNATURES } from '$lib/primitives';
	import ScatterPlot from '$lib/components/ScatterPlot.svelte';
	import { processYamlSchemes } from '$lib/data';
	import { getFilterStore, buildUrlParams, createFilterStore } from '$lib/filterStore';
	import { roundStore, roundToFilter, type Round } from '$lib/roundStore';
	import { allSchemeData, benchmarkEnv } from '$lib/schemeData';
	import { lastDataChange } from '$lib/dataDates';
	import type { PageData } from './$types';

	let { data: _ }: { data: PageData } = $props();

	const _init = processYamlSchemes(allSchemeData, roundToFilter('latest').filter, { useLatestVersion: true, includeOtherProgrammes: true });
	let schemes = $state(_init.schemes);
	let categories = $state([...new Set(_init.schemes.map((s) => s.category))].sort());
	let ranges = $state(_init.ranges);
	let groups = $state(presentProgrammeGroups(_init.parameterSets));
	const { store, filteredRows } = getFilterStore();

	function applyRound(round: Round, urlParams: URLSearchParams = new URLSearchParams()) {
		const { filter, useLatestVersion, includeOtherProgrammes } = roundToFilter(round);
		const result = processYamlSchemes(allSchemeData, filter, { useLatestVersion, includeOtherProgrammes });
		schemes = result.schemes;
		categories = [...new Set(result.schemes.map((s) => s.category))].sort();
		ranges = result.ranges;
		groups = presentProgrammeGroups(result.parameterSets);
		createFilterStore(result.parameterSets, result.ranges, urlParams);
	}

	function roundFromUrl(params: URLSearchParams): Round {
		switch (params.get('r')) {
			case '1': return 'round-1';
			case '2': return 'round-2';
			case '3': return 'round-3';
			default: return 'latest';
		}
	}

	onMount(() => {
		// The URL is the source of truth on load: pick the round first, then build the
		// filter store for that round with the URL's filter params already applied.
		const params = new URLSearchParams(window.location.search);
		let currentRound = roundFromUrl(params);
		roundStore.set(currentRound);
		applyRound(currentRound, params);

		// Debounced URL sync on filter changes. Defaults are read at sync time because
		// they change whenever applyRound() builds a store for another round.
		let urlSyncTimer: ReturnType<typeof setTimeout> | null = null;
		const unsubFilter = getFilterStore().store.subscribe((state) => {
			if (urlSyncTimer) clearTimeout(urlSyncTimer);
			urlSyncTimer = setTimeout(() => {
				const p = buildUrlParams(state, getFilterStore().defaults);
				const r = get(roundStore);
				const round = r === 'round-1' ? '1' : r === 'round-2' ? '2' : r === 'round-3' ? '3' : null;
				if (round) p.set('r', round);
				const qs = p.toString();
				const target = urlSyncTarget(
					qs,
					$page.url.pathname,
					window.location.hash,
					window.location.search,
					navigating.to !== null
				);
				if (target === null) return;
				goto(target, { replaceState: true, keepFocus: true, noScroll: true });
			}, 300);
		});

		// Round changes after load keep the common filters (see below). subscribe() fires
		// synchronously with the current value; the round-equality guard skips that call so
		// it cannot wipe the URL-applied filters.
		const unsubRound = roundStore.subscribe((round) => {
			if (round === currentRound) return;
			currentRound = round;
			// Keep the data-independent filters (programmes, levels, parameter sets, status,
			// chart) and the sort; scheme and range filters reset to the new round's data.
			const { store: s, defaults } = getFilterStore();
			const current = get(s);
			const carry = new URLSearchParams();
			encodeCommonFilters(current, defaults, carry);
			applyRound(round, carry);
			getFilterStore().store.update((f) => ({ ...f, sortCol: current.sortCol, sortDir: current.sortDir }));
		});

		return () => {
			unsubRound();
			unsubFilter();
			if (urlSyncTimer) clearTimeout(urlSyncTimer);
		};
	});
</script>

<svelte:head>
	<title>Signatures · {SITE_NAME}</title>
</svelte:head>

<div class="mx-auto max-w-[120rem] px-6 py-8">
	<!-- Hero header -->
	<div class="mb-8 border-l-4 border-pqs-apricot pl-4">
		<h1 class="font-heading text-3xl font-bold text-pqs-midnight dark:text-white">
			Post-Quantum Signature Schemes
		</h1>
		<p class="mt-2 text-sm text-pqs-steel dark:text-pqs-bluegray">
			Standardised schemes and candidates from the NIST on-ramp and other standardisation programmes,
			with pre-quantum schemes for reference. Select column headers to sort; use the filters to narrow
			by programme, scheme, security level, size or speed.
		</p>
		<p class="mt-1.5 text-xs text-pqs-steel/70 dark:text-pqs-bluegray/70">
			{#if $roundStore === 'latest'}
				Data reflects the latest known specifications for each scheme, last updated {lastDataChange}.
				Each scheme page links to the scheme's website and lists its recorded versions.
			{:else if $roundStore === 'round-3'}
				Showing the 9 schemes selected for Round 3 of the NIST Additional Signatures competition.
				Round 3 specific submission data is not yet available; data reflects the most recent known specifications (last updated {lastDataChange}).
			{:else}
				Data reflects scheme specifications as submitted at the start of {$roundStore === 'round-1' ? 'Round 1' : 'Round 2'} of the NIST Additional Signatures competition (data last updated {lastDataChange}).
				Schemes may have been updated since; see the individual scheme pages for later versions.
			{/if}
		</p>
		<p class="mt-1 text-xs text-pqs-steel/70 dark:text-pqs-bluegray/70">
			Latest also includes schemes from other programmes; Round 1–3 show only that round's on-ramp submissions, plus NIST standards and pre-quantum references.
		</p>
	</div>

	{#snippet onrampRound(enabled: boolean)}
		<RoundSelect disabled={!enabled} />
	{/snippet}

	<div class="flex flex-col gap-6 lg:flex-row lg:gap-8">
		<FilterSidebar summary={filterSummary($store, $store.schemes.size)}>
			<FilterPanel {schemes} {categories} {ranges} {groups} onramp={onrampRound} />
		</FilterSidebar>

		<!-- Main content -->
		<div class="min-w-0 flex-1 space-y-6">
			<!-- Scatter plot -->
			<section class="rounded border border-pqs-ashgray bg-white p-4 shadow-sm dark:border-pqs-steel dark:bg-pqs-midnight-mid">
				<div class="mb-3 flex flex-wrap items-center justify-between gap-3">
					<h2 class="font-heading text-base font-semibold text-pqs-steel dark:text-pqs-apricot">
						{$store.chart === 'speed' ? 'Signing vs. verification time' : 'pk size vs. sig size'}
						<span class="font-normal text-pqs-bluegray">(log–log scale)</span>
					</h2>
					<ChartModeToggle mode={$store.chart} onchange={(chart) => store.update((f) => ({ ...f, chart }))} />
				</div>
				<ScatterPlot
					xField={$store.chart === 'speed' ? 'signingTime' : 'pk'}
					yField={$store.chart === 'speed' ? 'verificationTime' : 'sig'}
				/>
				<div class="mt-2 flex justify-end">
					<a href="{base}/signatures/advanced/" class="text-xs text-pqs-steel hover:text-pqs-apricot dark:text-pqs-bluegray dark:hover:text-pqs-apricot transition-colors">
						Advanced graph →
					</a>
				</div>
			</section>

			<!-- Performance disclaimer -->
			<div class="rounded border border-pqs-apricot/40 bg-pqs-apricot/10 px-4 py-3 text-xs text-pqs-midnight dark:border-pqs-apricot/30 dark:bg-pqs-apricot/5 dark:text-pqs-smoke">
				<strong class="font-heading font-semibold text-pqs-apricot">Performance note:</strong>
				{#if $roundStore === 'latest' || $roundStore === 'round-3'}
					Cycle counts are from our own benchmarks on a {benchmarkEnv?.cpu.model ?? 'reference machine'} —
					see the environment details below.
				{:else}
					Timings are taken from the scheme submissions and may not reflect optimised implementations.
					Comparisons across schemes should be treated with caution.
				{/if}
			</div>

			<!-- Unified table -->
			<section>
				<CompareTable
					config={SIGNATURES}
					rows={$filteredRows}
					sortCol={$store.sortCol}
					sortDir={$store.sortDir}
					onsort={(sortCol, sortDir) => store.update((f) => ({ ...f, sortCol, sortDir }))}
				/>
			</section>

			<!-- Benchmark environment -->
			{#if benchmarkEnv}
				<BenchmarkEnvInfo env={benchmarkEnv} />
			{/if}
		</div>
	</div>
</div>
