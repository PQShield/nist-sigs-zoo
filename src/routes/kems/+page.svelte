<script lang="ts">
	import { SITE_NAME } from '$lib/constants';
	import ComparePage from '$lib/components/ComparePage.svelte';
	import { kemCompareData } from '$lib/compareData';
	import { KEMS } from '$lib/primitives';
	import { base } from '$app/paths';
	import { kemBenchmarkEnv } from '$lib/kemSchemeData';
</script>

<svelte:head>
	<title>KEMs · {SITE_NAME}</title>
</svelte:head>

{#snippet hero()}
	<div class="mb-8 border-l-4 border-pqs-apricot pl-4">
		<h1 class="font-heading text-3xl font-bold text-pqs-midnight dark:text-white">
			Key Encapsulation Mechanisms
		</h1>
		<p class="mt-2 text-sm text-pqs-steel dark:text-pqs-bluegray">
			Standardised KEMs, competition candidates and research proposals, with pre-quantum ECDH for
			reference. Select column headers to sort; use the filters to narrow by programme, scheme,
			security level, size or speed.
		</p>
		<p class="mt-1.5 text-xs text-pqs-steel/70 dark:text-pqs-bluegray/70">
			Sizes reflect the latest known specifications (ML-KEM: FIPS 203; HQC: 2025-08-22 draft). For
			ECDH the “ciphertext” is the ephemeral public key and keygen/encaps/decaps map to the
			static/ephemeral key generation and the two ECDH derivations.
			See the <a href="{base}/signatures/" class="text-pqs-apricot hover:underline">signature comparison</a>
			for signature schemes.
		</p>
	</div>
{/snippet}

{#snippet note()}
	{#if kemBenchmarkEnv}
		<div class="rounded border border-pqs-apricot/40 bg-pqs-apricot/10 px-4 py-3 text-xs text-pqs-midnight dark:border-pqs-apricot/30 dark:bg-pqs-apricot/5 dark:text-pqs-smoke">
			<strong class="font-heading font-semibold text-pqs-apricot">Performance note:</strong>
			Keygen / encapsulation / decapsulation timings are from our own benchmarks on a
			{kemBenchmarkEnv.cpu.model} (ML-KEM and HQC use AVX2; ECDH uses OpenSSL); see the
			environment details below. Schemes not yet in our harness show sizes only.
		</div>
	{/if}
{/snippet}

<ComparePage config={KEMS} load={kemCompareData} {hero} {note} env={kemBenchmarkEnv} />
