<script lang="ts">
	import { SITE_NAME } from '$lib/constants';
	import { fmtDate } from '$lib/dates';
	import FlagNotes from '$lib/components/scheme/FlagNotes.svelte';
	import ParamSetTable from '$lib/components/scheme/ParamSetTable.svelte';
	import VersionCompare from '$lib/components/scheme/VersionCompare.svelte';
	import SchemeEvents from '$lib/components/scheme/SchemeEvents.svelte';
	import SchemeHeader from '$lib/components/scheme/SchemeHeader.svelte';
	import type { PageData } from './$types';

	let { data }: { data: PageData } = $props();
	const page = $derived(data.page);
</script>

<svelte:head>
	<title>{page.name} · {SITE_NAME}</title>
	<meta
		name="description"
		content="{page.name}: parameter sets, sizes, timings, programmes, security flags and history."
	/>
</svelte:head>

<!-- Keyed so that back/forward between two scheme pages resets per-page state (e.g. the version selects). -->
{#key page.slug}
	<div class="mx-auto max-w-screen-lg px-4 py-8 sm:px-6">
		<SchemeHeader {page} />

		<section aria-labelledby="sets-heading" class="mt-10">
			<h2 id="sets-heading" class="font-heading text-xl font-bold text-pqs-midnight dark:text-pqs-smoke">
				Parameter sets
			</h2>
			{#if page.primitive === 'signature'}
				<p class="mt-1 text-sm text-pqs-steel dark:text-pqs-bluegray">
					{page.latest.label} · {fmtDate(page.latest.date)}
				</p>
			{/if}
			<ParamSetTable table={page.table} caption="Parameter sets of {page.name}" />
		</section>

		<FlagNotes notes={page.flagNotes} />

		{#if page.primitive === 'signature'}
			{#if page.options.length > 1}
				<VersionCompare versions={page.versions} options={page.options} />
			{:else}
				<p class="mt-10 text-sm text-pqs-steel dark:text-pqs-bluegray">
					Only one version of {page.name} is recorded, so there is nothing to compare yet.
				</p>
			{/if}
		{/if}

		<SchemeEvents events={page.events} />
	</div>
{/key}
