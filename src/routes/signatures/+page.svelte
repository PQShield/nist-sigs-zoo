<script lang="ts">
	import { goto } from '$app/navigation';
	import { page } from '$app/stores';
	import { base } from '$app/paths';
	import { onMount } from 'svelte';
	import { get } from 'svelte/store';
	import BenchmarkEnvInfo from '$lib/components/BenchmarkEnvInfo.svelte';
	import RoundSelect from '$lib/components/RoundSelect.svelte';
	import FilterSidebar from '$lib/components/FilterSidebar.svelte';
	import { filterSummary, presentProgrammeGroups } from '$lib/compareFilters';
	import FilterPanel from '$lib/components/FilterPanel.svelte';
	import SchemeTable from '$lib/components/SchemeTable.svelte';
	import ScatterPlot from '$lib/components/ScatterPlot.svelte';
	import { processYamlSchemes } from '$lib/data';
	import { getFilterStore, buildUrlParams, createFilterStore } from '$lib/filterStore';
	import { roundStore, roundToFilter, type Round } from '$lib/roundStore';
	import { allSchemeData, benchmarkEnv, lastUpdated } from '$lib/schemeData';
	import type { PageData } from './$types';

	let { data: _ }: { data: PageData } = $props();

	const _init = processYamlSchemes(allSchemeData, roundToFilter('latest').filter, { useLatestVersion: true, includeOtherProgrammes: true });
	let schemes = $state(_init.schemes);
	let categories = $state([...new Set(_init.schemes.map((s) => s.category))].sort());
	let ranges = $state(_init.ranges);
	let groups = $state(presentProgrammeGroups(_init.parameterSets));
	const { store } = getFilterStore();

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
				const newUrl = (qs ? `?${qs}` : $page.url.pathname) + window.location.hash;
				goto(newUrl, { replaceState: true, keepFocus: true, noScroll: true });
			}, 300);
		});

		// Round changes after load reset filters to that round's defaults. subscribe() fires
		// synchronously with the current value; the round-equality guard skips that call so
		// it cannot wipe the URL-applied filters.
		const unsubRound = roundStore.subscribe((round) => {
			if (round === currentRound) return;
			currentRound = round;
			applyRound(round);
		});

		return () => {
			unsubRound();
			unsubFilter();
			if (urlSyncTimer) clearTimeout(urlSyncTimer);
		};
	});
</script>

<div class="mx-auto max-w-screen-2xl px-6 py-8">
	<!-- Hero header -->
	<div class="mb-8 border-l-4 border-pqs-apricot pl-4">
		<h1 class="font-heading text-3xl font-bold text-pqs-midnight dark:text-white">
			Post-Quantum Signature Schemes
		</h1>
		<p class="mt-2 text-sm text-pqs-steel dark:text-pqs-bluegray">
			Comparing NIST on-ramp candidates and standardized schemes.
			Click column headers to sort. Use the filters to narrow down by category, security level, or size constraints.
		</p>
		<p class="mt-1.5 text-xs text-pqs-steel/70 dark:text-pqs-bluegray/70">
			{#if $roundStore === 'latest'}
				Data reflects the latest known specifications for each scheme, last updated {lastUpdated}.
				Consult the individual scheme websites for the most current information.
			{:else if $roundStore === 'round-3'}
				Showing the 9 schemes selected for Round 3 of the NIST Additional Signatures competition.
				Round 3 specific submission data is not yet available; data reflects the most recent known specifications (last updated {lastUpdated}).
			{:else}
				Data reflects scheme specifications as submitted at the start of {$roundStore === 'round-1' ? 'Round 1' : 'Round 2'} of the NIST Additional Signatures competition (data last updated {lastUpdated}).
				Schemes may have been updated since; consult the individual scheme websites for current specifications.
			{/if}
		</p>
		<div class="mt-3"><RoundSelect /></div>
	</div>

	<div class="flex flex-col gap-6 lg:flex-row lg:gap-8">
		<FilterSidebar summary={filterSummary($store, $store.schemes.size)}>
			<FilterPanel {schemes} {categories} {ranges} {groups} />
		</FilterSidebar>

		<!-- Main content -->
		<div class="min-w-0 flex-1 space-y-6">
			<!-- Scatter plot -->
			<section class="rounded border border-pqs-ashgray bg-white p-4 shadow-sm dark:border-pqs-steel dark:bg-pqs-midnight-mid">
				<h2 class="mb-3 font-heading text-base font-semibold text-pqs-steel dark:text-pqs-apricot">
					pk size vs. sig size <span class="font-normal text-pqs-bluegray">(log–log scale)</span>
				</h2>
				<ScatterPlot />
				<div class="mt-2 flex justify-end">
					<a href="{base}/signatures/advanced/" class="text-xs text-pqs-bluegray hover:text-pqs-apricot dark:text-pqs-steel dark:hover:text-pqs-apricot transition-colors">
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
				<SchemeTable />
			</section>

			<!-- Benchmark environment -->
			{#if benchmarkEnv}
				<BenchmarkEnvInfo env={benchmarkEnv} />
			{/if}
		</div>
	</div>
</div>
