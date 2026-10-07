<script lang="ts">
	import { onMount } from 'svelte';
	import { base } from '$app/paths';
	import { legacyHomeRedirect } from '$lib/legacy';

	onMount(() => {
		const target = legacyHomeRedirect(location.search, location.hash, base);
		if (target) location.replace(target);
	});
</script>

<div class="mx-auto max-w-screen-2xl px-6 py-8">
	<a href="{base}/signatures/">Signatures</a>
</div>
