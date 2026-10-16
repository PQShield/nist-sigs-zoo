<script lang="ts">
	import { base } from '$app/paths';
	import { SITE_NAME } from '$lib/constants';
	import type { PageData } from './$types';

	let { data }: { data: PageData } = $props();

	const count = (n: number | null, one: string, many: string) =>
		n === null ? `${many}: not in scope` : `${n} ${n === 1 ? one : many} in our data`;
</script>

<svelte:head>
	<title>Programmes · {SITE_NAME}</title>
</svelte:head>

<div class="mx-auto max-w-[120rem] px-6 py-8">
	<div class="mb-8 border-l-4 border-pqs-apricot pl-4">
		<h1 class="font-heading text-3xl font-bold text-pqs-midnight dark:text-white">Programmes</h1>
		<p class="mt-2 max-w-[72ch] text-sm text-pqs-steel dark:text-pqs-bluegray">
			Standardisation programmes and competitions whose candidates we track. Each page has the programme's timeline, its
			security-level scale and its candidates.
		</p>
	</div>
	<ul class="grid gap-4 sm:grid-cols-2 lg:grid-cols-3">
		{#each data.cards as c (c.programme.id)}
			<li class="rounded border border-pqs-ashgray bg-white p-5 dark:border-pqs-steel dark:bg-pqs-midnight-mid">
				<h2 class="font-heading text-xl font-bold">
					<a
						href="{base}/programmes/{c.programme.id}/"
						class="text-pqs-steel hover:text-pqs-apricot dark:text-pqs-apricot"
					>
						{c.programme.nav_label ?? c.programme.short}
					</a>
				</h2>
				<p class="mt-1 text-sm">{c.programme.name}</p>
				<p class="mt-1 text-xs text-pqs-steel dark:text-pqs-bluegray">
					{c.programme.organiser} · {c.programme.region}{#if c.stage}{' · '}{c.stage}{/if}
				</p>
				<p class="mt-3 text-sm">{count(c.signatures, 'signature', 'signatures')} · {count(c.kems, 'KEM', 'KEMs')}</p>
			</li>
		{/each}
	</ul>
</div>
