<script lang="ts">
	import { goto } from '$app/navigation';
	import { page } from '$app/stores';
	import { onMount } from 'svelte';
	import FilterPanel from '$lib/components/FilterPanel.svelte';
	import SchemeTable from '$lib/components/SchemeTable.svelte';
	import ScatterPlot from '$lib/components/ScatterPlot.svelte';
	import { processYamlSchemes } from '$lib/data';
	import { getFilterStore, buildUrlParams, createFilterStore } from '$lib/filterStore';
	import { roundStore, type Round } from '$lib/roundStore';
	import { allSchemeData, lastUpdated } from '$lib/schemeData';
	import type { PageData } from './$types';

	let { data: _ }: { data: PageData } = $props();

	const _init = processYamlSchemes(allSchemeData, 'round-3', { useLatestVersion: true });
	let schemes = $state(_init.schemes);
	let categories = $state([...new Set(_init.schemes.map((s) => s.category))].sort());
	let ranges = $state(_init.ranges);

	const { applyUrl } = getFilterStore();

	function applyRound(round: Round) {
		const tagFilter = round === 'latest' || round === 'round-3' ? 'round-3' : round;
		const result = processYamlSchemes(allSchemeData, tagFilter, { useLatestVersion: round === 'latest' || round === 'round-3' });
		schemes = result.schemes;
		categories = [...new Set(result.schemes.map((s) => s.category))].sort();
		ranges = result.ranges;
		createFilterStore(result.parameterSets, result.ranges, new URLSearchParams());
	}

	onMount(() => {
		// Apply round from URL (?r=1 or ?r=2)
		const params = new URLSearchParams(window.location.search);
		const rParam = params.get('r');
		if (rParam === '1') {
			roundStore.set('round-1');
			applyRound('round-1');
		} else if (rParam === '2') {
			roundStore.set('round-2');
			applyRound('round-2');
		} else if (rParam === '3') {
			roundStore.set('round-3');
			applyRound('round-3');
		}

		// Apply filter URL params
		if (params.toString()) applyUrl(params);

		// Debounced URL sync on filter changes
		let urlSyncTimer: ReturnType<typeof setTimeout> | null = null;
		const { store, defaults } = getFilterStore();
		const unsubFilter = store.subscribe((state) => {
			if (urlSyncTimer) clearTimeout(urlSyncTimer);
			urlSyncTimer = setTimeout(() => {
				const p = buildUrlParams(state, defaults);
				const round = $roundStore === 'round-1' ? '1' : $roundStore === 'round-2' ? '2' : $roundStore === 'round-3' ? '3' : null;
				if (round) p.set('r', round);
				const qs = p.toString();
				const newUrl = qs ? `?${qs}` : $page.url.pathname;
				goto(newUrl, { replaceState: true, keepFocus: true, noScroll: true });
			}, 300);
		});

		// Re-process when round changes — store stays stable, data updates in place
		const unsubRound = roundStore.subscribe((round) => {
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
	</div>

	<div class="flex gap-8">
		<!-- Sidebar filter panel -->
		<div class="hidden w-60 shrink-0 lg:block">
			<div class="sticky top-4">
				<FilterPanel schemes={schemes} categories={categories} ranges={ranges} />
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
					<FilterPanel schemes={schemes} categories={categories} ranges={ranges} />
				</div>
			</details>

			<!-- Scatter plot -->
			<section class="rounded border border-pqs-ashgray bg-white p-4 shadow-sm dark:border-pqs-steel dark:bg-pqs-midnight-mid">
				<h2 class="mb-3 font-heading text-base font-semibold text-pqs-steel dark:text-pqs-apricot">
					pk size vs. sig size <span class="font-normal text-pqs-bluegray">(log–log scale)</span>
				</h2>
				<ScatterPlot />
			</section>

			<!-- Performance disclaimer -->
			<div class="rounded border border-pqs-apricot/40 bg-pqs-apricot/10 px-4 py-3 text-xs text-pqs-midnight dark:border-pqs-apricot/30 dark:bg-pqs-apricot/5 dark:text-pqs-smoke">
				<strong class="font-heading font-semibold text-pqs-apricot">Performance note:</strong>
				Timings are taken from the scheme submissions and may not reflect optimised implementations.
				Cycle counts marked with a
				<span class="underline decoration-wavy decoration-pqs-scarlet">wavy underline</span>
				are extrapolated from reported millisecond timings assuming a 2.5 GHz processor —
				comparisons across such values should be treated with caution.
			</div>

			<!-- Unified table -->
			<section>
				<SchemeTable />
			</section>
		</div>
	</div>
</div>
