<script lang="ts">
	import { goto } from '$app/navigation';
	import { page } from '$app/stores';
	import { base } from '$app/paths';
	import { onMount } from 'svelte';
	import KemFilterPanel from '$lib/components/KemFilterPanel.svelte';
	import KemTable from '$lib/components/KemTable.svelte';
	import KemScatterPlot from '$lib/components/KemScatterPlot.svelte';
	import { allKemData, processKems } from '$lib/kemData';
	import { getKemStore, buildUrlParams, createKemStore } from '$lib/kemStore';
	import type { PageData } from './$types';

	let { data: _ }: { data: PageData } = $props();

	const _init = processKems(allKemData);
	const kems = _init.kems;
	const categories = [...new Set(_init.kems.map((k) => k.category))].sort();

	createKemStore(_init.parameterSets, new URLSearchParams());

	const { applyUrl } = getKemStore();

	onMount(() => {
		const params = new URLSearchParams(window.location.search);
		if (params.toString()) applyUrl(params);

		let urlSyncTimer: ReturnType<typeof setTimeout> | null = null;
		const { store, defaults } = getKemStore();
		const unsubFilter = store.subscribe((state) => {
			if (urlSyncTimer) clearTimeout(urlSyncTimer);
			urlSyncTimer = setTimeout(() => {
				const p = buildUrlParams(state, defaults);
				const qs = p.toString();
				const newUrl = qs ? `?${qs}` : $page.url.pathname;
				goto(newUrl, { replaceState: true, keepFocus: true, noScroll: true });
			}, 300);
		});

		return () => {
			unsubFilter();
			if (urlSyncTimer) clearTimeout(urlSyncTimer);
		};
	});
</script>

<div class="mx-auto max-w-screen-2xl px-6 py-8">
	<div class="mb-8 border-l-4 border-pqs-apricot pl-4">
		<div class="flex items-baseline gap-3">
			<h1 class="font-heading text-3xl font-bold text-pqs-midnight dark:text-white">
				Post-Quantum KEMs
			</h1>
			<a href="{base}/" class="text-sm text-pqs-bluegray hover:text-pqs-apricot dark:text-pqs-steel dark:hover:text-pqs-apricot transition-colors">
				← Signatures
			</a>
		</div>
		<p class="mt-2 text-sm text-pqs-steel dark:text-pqs-bluegray">
			Comparing key sizes of key encapsulation mechanisms (KEMs).
			Click column headers to sort. Use the filters to narrow down by category or security level.
		</p>
		<p class="mt-1.5 text-xs text-pqs-steel/70 dark:text-pqs-bluegray/70">
			Sizes are in bytes. <strong>pk</strong> is the public key sent by the recipient,
			<strong>ct</strong> is the ciphertext sent back by the sender, and
			<strong>sk</strong> is the secret key held by the recipient.
			Timings are median keygen / encapsulation / decapsulation from our own benchmarks
			(see <code>kembench/</code>); cells without measured data are blank.
		</p>
	</div>

	<div class="flex gap-8">
		<div class="hidden w-60 shrink-0 lg:block">
			<div class="sticky top-4">
				<KemFilterPanel {kems} {categories} />
			</div>
		</div>

		<div class="min-w-0 flex-1 space-y-6">
			<details class="lg:hidden">
				<summary class="cursor-pointer rounded border border-pqs-bluegray bg-white px-3 py-2 font-heading text-sm font-semibold text-pqs-steel dark:border-pqs-steel dark:bg-pqs-midnight-mid dark:text-pqs-apricot">
					Filters ▼
				</summary>
				<div class="mt-2 rounded border border-pqs-ashgray bg-white p-3 dark:border-pqs-steel dark:bg-pqs-midnight-mid">
					<KemFilterPanel {kems} {categories} />
				</div>
			</details>

			<section class="rounded border border-pqs-ashgray bg-white p-4 shadow-sm dark:border-pqs-steel dark:bg-pqs-midnight-mid">
				<h2 class="mb-3 font-heading text-base font-semibold text-pqs-steel dark:text-pqs-apricot">
					pk size vs. ct size <span class="font-normal text-pqs-bluegray">(log–log scale)</span>
				</h2>
				<KemScatterPlot />
			</section>

			<section>
				<KemTable />
			</section>
		</div>
	</div>
</div>
