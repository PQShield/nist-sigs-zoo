<script lang="ts">
	import { SITE_NAME } from '#lib/constants.ts';
	import ComparePage from '#lib/components/ComparePage.svelte';
	import { signatureCompareData } from '#lib/compareData.ts';
	import { SIGNATURES } from '#lib/primitives.ts';
</script>

<svelte:head>
	<title>Signatures advanced graph · {SITE_NAME}</title>
</svelte:head>

<ComparePage config={SIGNATURES} load={signatureCompareData} advanced />
