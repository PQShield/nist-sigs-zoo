<script lang="ts">
	import { SITE_NAME } from '$lib/constants';
	import ComparePage from '$lib/components/ComparePage.svelte';
	import { kemCompareData } from '$lib/compareData';
	import { KEMS } from '$lib/primitives';
</script>

<svelte:head>
	<title>KEMs advanced graph · {SITE_NAME}</title>
</svelte:head>

<ComparePage config={KEMS} load={kemCompareData} advanced />
