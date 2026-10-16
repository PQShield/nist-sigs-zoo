<script lang="ts">
	import ProgrammeView from '$lib/components/programme/ProgrammeView.svelte';
	import type { PageData } from './$types';

	let { data }: { data: PageData } = $props();
</script>

<!-- Remount per programme: ProgrammeView and the candidate sections initialise once from their props. -->
{#key data.programme.id}
	<ProgrammeView programme={data.programme} />
{/key}
