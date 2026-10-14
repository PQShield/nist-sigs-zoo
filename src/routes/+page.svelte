<script lang="ts">
	import { onMount } from 'svelte';
	import { base } from '$app/paths';
	import { legacyHomeRedirect } from '$lib/legacy';
	import { SITE_NAME } from '$lib/constants';
	import { lastUpdated } from '$lib/schemeData';
	import { summariseGroups, type StageGroup } from '$lib/programmes';
	import type { PageData } from './$types';

	let { data }: { data: PageData } = $props();

	onMount(() => {
		const target = legacyHomeRedirect(location.search, location.hash, base);
		if (target) location.replace(target);
	});

	const cap = (t: string) => t.charAt(0).toUpperCase() + t.slice(1);
</script>

{#snippet cell(groups: StageGroup[])}
	{#if groups.length === 0}
		—
	{:else}
		{@const { full, ended } = summariseGroups(groups)}
		{#each full as g (g.stage.id)}
			<p><span class="font-semibold">{cap(g.stage.label)}:</span> {g.schemes.join(', ')}</p>
		{/each}
		{#each ended as e (e.stage.id)}
			<p class="text-xs text-pqs-steel dark:text-pqs-bluegray">Ended in {e.stage.id.replace('round-', 'round ')}: {e.count} {e.count === 1 ? 'scheme' : 'schemes'}</p>
		{/each}
	{/if}
{/snippet}

<svelte:head>
	<title>{SITE_NAME}</title>
</svelte:head>

<section class="bg-pqs-midnight text-pqs-smoke">
	<div class="mx-auto max-w-[120rem] px-6 py-16">
		<h1 class="max-w-[18ch] font-heading text-4xl font-bold leading-tight md:text-5xl">
			Post-quantum schemes, compared across standards programmes.
		</h1>
		<p class="mt-5 max-w-[58ch] text-lg text-pqs-bluegray">
			Sizes, benchmarks and security status for signatures and KEMs from NIST, China's ICCS,
			Korea's KpqC and ISO, updated as specs change and attacks land.
		</p>
		<div class="mt-8 flex flex-wrap gap-3 font-heading font-semibold">
			<a href="{base}/signatures/" class="rounded bg-pqs-apricot px-5 py-3 text-pqs-midnight">Compare signatures</a>
			<a href="{base}/kems/" class="rounded border border-pqs-bluegray px-5 py-3 text-white">Compare KEMs</a>
		</div>
		<dl class="mt-12 grid grid-cols-2 gap-6 border-t border-pqs-steel/40 pt-6 font-heading sm:grid-cols-4">
			<div class="flex flex-col-reverse"><dt class="text-sm text-pqs-bluegray">signature schemes</dt><dd class="text-3xl font-bold">{data.signatureCount}</dd></div>
			<div class="flex flex-col-reverse"><dt class="text-sm text-pqs-bluegray">KEMs</dt><dd class="text-3xl font-bold">{data.kemCount}</dd></div>
			<div class="flex flex-col-reverse"><dt class="text-sm text-pqs-bluegray">programmes</dt><dd class="text-3xl font-bold">{data.summaries.length}</dd></div>
			<div class="flex flex-col-reverse"><dt class="text-sm text-pqs-bluegray">last data change</dt><dd class="text-3xl font-bold">{lastUpdated}</dd></div>
		</dl>
	</div>
</section>

<div class="mx-auto max-w-[120rem] space-y-14 px-6 py-14">
	<section aria-labelledby="programmes-h">
		<h2 id="programmes-h" class="mb-4 font-heading text-2xl font-bold">Programmes</h2>
		<div class="overflow-x-auto rounded border border-pqs-ashgray bg-white dark:border-pqs-steel dark:bg-pqs-midnight-mid">
			<table class="min-w-full text-sm">
				<thead>
					<tr class="bg-pqs-steel font-heading text-xs text-white">
						<th scope="col" class="px-3 py-2.5 text-left font-semibold">Programme</th>
						<th scope="col" class="px-3 py-2.5 text-left font-semibold">Signatures</th>
						<th scope="col" class="px-3 py-2.5 text-left font-semibold">KEMs</th>
					</tr>
				</thead>
				<tbody class="divide-y divide-pqs-ashgray dark:divide-pqs-steel">
					{#each data.summaries as s (s.programme.id)}
						<tr>
							<td class="px-3 py-2">
								<a href="{base}/programmes/{s.programme.id}/" class="font-heading font-semibold text-pqs-steel underline hover:text-pqs-apricot dark:text-pqs-apricot">{s.programme.name}</a>
								<span class="block text-xs text-pqs-steel dark:text-pqs-bluegray">{[s.programme.short, ...(s.programme.organiser === s.programme.short ? [] : [s.programme.organiser]), s.programme.region].join(' · ')}</span>
							</td>
							<td class="px-3 py-2">{#if s.programme.primitives.includes('signature')}{@render cell(s.signatures)}{:else}Not in scope{/if}</td>
							<td class="px-3 py-2">{#if s.programme.primitives.includes('kem')}{@render cell(s.kems)}{:else}Not in scope{/if}</td>
						</tr>
					{/each}
				</tbody>
			</table>
		</div>
	</section>

	<section aria-labelledby="latest-h" class="mx-auto max-w-[880px]">
		<div class="mb-2 flex items-baseline justify-between">
			<h2 id="latest-h" class="font-heading text-2xl font-bold">Latest changes</h2>
			<a href="{base}/timeline/" class="font-heading text-sm font-semibold text-pqs-steel dark:text-pqs-apricot">Full timeline</a>
		</div>
		<ol class="divide-y divide-pqs-ashgray border-y border-pqs-ashgray dark:divide-pqs-steel dark:border-pqs-steel">
			{#each data.latest as e (e.date + e.description)}
				<li class="grid gap-1 py-4 sm:grid-cols-[112px_1fr] sm:gap-6">
					<div class="font-heading text-sm">
						<time class="block text-pqs-steel dark:text-pqs-bluegray">{e.date}</time>
						<span class="font-semibold capitalize">{e.type}</span>
					</div>
					<p class="text-sm [&_a]:text-pqs-steel [&_a]:underline dark:[&_a]:text-pqs-apricot">{@html e.description}</p>
				</li>
			{/each}
		</ol>
	</section>
</div>
