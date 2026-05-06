<script lang="ts">
	import { browser } from '$app/environment';
	import { goto } from '$app/navigation';
	import { page } from '$app/stores';
	import { onMount } from 'svelte';
	import FilterPanel from '$lib/components/FilterPanel.svelte';
	import SchemeTable from '$lib/components/SchemeTable.svelte';
	import ScatterPlot from '$lib/components/ScatterPlot.svelte';
	import { getFilterStore, buildUrlParams } from '$lib/filterStore';
	import type { PageData } from './$types';

	let { data }: { data: PageData } = $props();

	const { store, defaults, applyUrl } = getFilterStore();

	onMount(() => {
		// Apply URL params now that we're on the client
		const params = new URLSearchParams(window.location.search);
		if (params.toString()) applyUrl(params);

		// Debounced URL sync on subsequent filter changes
		let urlSyncTimer: ReturnType<typeof setTimeout> | null = null;
		const unsub = store.subscribe((state) => {
			if (urlSyncTimer) clearTimeout(urlSyncTimer);
			urlSyncTimer = setTimeout(() => {
				const p = buildUrlParams(state, defaults);
				const qs = p.toString();
				const newUrl = qs ? `?${qs}` : $page.url.pathname;
				goto(newUrl, { replaceState: true, keepFocus: true, noScroll: true });
			}, 300);
		});
		return () => {
			unsub();
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
			Data reflects scheme specifications as submitted at the start of Round 2 of the NIST Additional Signatures competition.
			Schemes may have been updated since; consult the individual scheme websites for current specifications.
		</p>
	</div>

	<div class="flex gap-8">
		<!-- Sidebar filter panel -->
		<div class="hidden w-60 shrink-0 lg:block">
			<div class="sticky top-4">
				<FilterPanel schemes={data.schemes} categories={data.categories} ranges={data.ranges} />
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
					<FilterPanel schemes={data.schemes} categories={data.categories} ranges={data.ranges} />
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
