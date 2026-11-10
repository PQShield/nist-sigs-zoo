<script lang="ts">
	import { SITE_NAME } from '#lib/constants.ts';
	import ComparePage from '#lib/components/ComparePage.svelte';
	import { kemCompareData } from '#lib/compareData.ts';
	import { KEMS } from '#lib/primitives.ts';
</script>

<svelte:head>
	<title>KEMs advanced graph · {SITE_NAME}</title>
</svelte:head>

<ComparePage config={KEMS} load={kemCompareData} advanced />
