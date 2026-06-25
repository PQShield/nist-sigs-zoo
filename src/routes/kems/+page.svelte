<script lang="ts">
	import { base } from '$app/paths';
	import KemFilterPanel from '$lib/components/KemFilterPanel.svelte';
	import KemScatterPlot from '$lib/components/KemScatterPlot.svelte';
	import KemTable from '$lib/components/KemTable.svelte';
	import {
		computeKemRanges,
		defaultKemFilter,
		filterKemRows,
		processKemSchemes
	} from '$lib/kemData';
	import BenchmarkEnvInfo from '$lib/components/BenchmarkEnvInfo.svelte';
	import { allKemData, kemBenchmarkEnv } from '$lib/kemSchemeData';
	import type { PageData } from './$types';

	// load (+page.ts) drives prerendering; data is read from the module glob here
	// (matching the signatures page) so the one-time filter init stays warning-free.
	let { data: _ }: { data: PageData } = $props();

	const { schemes, parameterSets } = processKemSchemes(allKemData);
	const ranges = computeKemRanges(parameterSets);
	const categories = [...new Set(schemes.map((s) => s.category))].sort();

	let filter = $state(defaultKemFilter(ranges, schemes.map((s) => s.scheme)));

	const filteredRows = $derived(filterKemRows(parameterSets, filter));
</script>

<div class="mx-auto max-w-screen-2xl px-6 py-8">
	<!-- Hero header -->
	<div class="mb-8 border-l-4 border-pqs-apricot pl-4">
		<h1 class="font-heading text-3xl font-bold text-pqs-midnight dark:text-white">
			Key Encapsulation Mechanisms
		</h1>
		<p class="mt-2 text-sm text-pqs-steel dark:text-pqs-bluegray">
			A standalone comparison of public-key and ciphertext sizes for KEMs — not tied to any NIST
			competition round. Click column headers to sort; use the filters to narrow down.
		</p>
		<p class="mt-1.5 text-xs text-pqs-steel/70 dark:text-pqs-bluegray/70">
			Sizes reflect the latest known specifications (ML-KEM: FIPS 203; HQC: 2025-08-22 draft). For
			ECDH the “ciphertext” is the ephemeral public key and keygen/encaps/decaps map to the
			static/ephemeral key generation and the two ECDH derivations.
			See the <a href="{base}/" class="text-pqs-apricot hover:underline">signatures comparison</a>
			for the NIST on-ramp signature schemes.
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
				<h2 class="mb-3 font-heading text-base font-semibold text-pqs-steel dark:text-pqs-apricot">
					pk size vs. ciphertext size <span class="font-normal text-pqs-bluegray">(log–log scale)</span>
				</h2>
				<KemScatterPlot rows={filteredRows} />
			</section>

			<!-- Performance note -->
			{#if kemBenchmarkEnv}
				<div class="rounded border border-pqs-apricot/40 bg-pqs-apricot/10 px-4 py-3 text-xs text-pqs-midnight dark:border-pqs-apricot/30 dark:bg-pqs-apricot/5 dark:text-pqs-smoke">
					<strong class="font-heading font-semibold text-pqs-apricot">Performance note:</strong>
					Keygen / encapsulation / decapsulation timings are from our own benchmarks on a
					{kemBenchmarkEnv.cpu.model} (ML-KEM and HQC use AVX2; ECDH uses OpenSSL) — see the
					environment details below.
				</div>
			{/if}

			<!-- Table -->
			<section>
				<KemTable rows={filteredRows} />
			</section>

			<!-- Benchmark environment -->
			{#if kemBenchmarkEnv}
				<BenchmarkEnvInfo env={kemBenchmarkEnv} />
			{/if}
		</div>
	</div>
</div>
