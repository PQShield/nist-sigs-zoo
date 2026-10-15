<script lang="ts">
	import { SITE_NAME } from '$lib/constants';
	import ComparePage from '$lib/components/ComparePage.svelte';
	import { signatureCompareData } from '$lib/compareData';
	import { SIGNATURES } from '$lib/primitives';
	import { roundStore } from '$lib/roundStore';
	import { envCpuModels } from '$lib/benchEnv';
	import { benchmarkEnv } from '$lib/schemeData';
	import { lastDataChange } from '$lib/dataDates';
</script>

<svelte:head>
	<title>Signatures · {SITE_NAME}</title>
</svelte:head>

{#snippet hero()}
	<div class="mb-8 border-l-4 border-pqs-apricot pl-4">
		<h1 class="font-heading text-3xl font-bold text-pqs-midnight dark:text-white">Post-Quantum Signature Schemes</h1>
		<p class="mt-2 text-sm text-pqs-steel dark:text-pqs-bluegray">
			Standardised schemes and candidates from the NIST on-ramp and other standardisation programmes, with pre-quantum
			schemes for reference. Select column headers to sort; use the filters to narrow by programme, scheme, security
			level, size or speed.
		</p>
		<p class="mt-1.5 text-xs text-pqs-steel/70 dark:text-pqs-bluegray/70">
			{#if $roundStore === 'latest'}
				Data reflects the latest known specifications for each scheme, last updated {lastDataChange}. Each scheme page
				links to the scheme's website and lists its recorded versions.
			{:else if $roundStore === 'round-3'}
				Showing the 9 schemes selected for Round 3 of the NIST Additional Signatures competition. Round 3 specific
				submission data is not yet available; data reflects the most recent known specifications (last updated {lastDataChange}).
			{:else}
				Data reflects scheme specifications as submitted at the start of {$roundStore === 'round-1'
					? 'Round 1'
					: 'Round 2'} of the NIST Additional Signatures competition (data last updated {lastDataChange}). Schemes may
				have been updated since; see the individual scheme pages for later versions.
			{/if}
		</p>
		<p class="mt-1 text-xs text-pqs-steel/70 dark:text-pqs-bluegray/70">
			Latest also includes schemes from other programmes; Round 1–3 show only that round's on-ramp submissions, plus
			NIST standards and pre-quantum references.
		</p>
	</div>
{/snippet}

{#snippet note()}
	<div
		class="rounded border border-pqs-apricot/40 bg-pqs-apricot/10 px-4 py-3 text-xs text-pqs-midnight dark:border-pqs-apricot/30 dark:bg-pqs-apricot/5 dark:text-pqs-smoke"
	>
		<strong class="font-heading font-semibold text-pqs-apricot">Performance note:</strong>
		{#if $roundStore === 'latest' || $roundStore === 'round-3'}
			Cycle counts are from our own benchmarks on a {envCpuModels(benchmarkEnv) ?? 'reference machine'} — see the environment
			details below.
		{:else}
			Timings are taken from the scheme submissions and may not reflect optimised implementations. Comparisons across
			schemes should be treated with caution.
		{/if}
	</div>
{/snippet}

<ComparePage config={SIGNATURES} load={signatureCompareData} rounds {hero} {note} env={benchmarkEnv} />
