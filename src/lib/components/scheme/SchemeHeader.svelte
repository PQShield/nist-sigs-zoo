<script lang="ts">
	import { base } from '$app/paths';
	import { fmtDate } from '$lib/dates';
	import type { SchemePageData } from '$lib/schemePage';
	import ProgrammeChips from '../ProgrammeChips.svelte';

	let { page }: { page: SchemePageData } = $props();

	const section = $derived(
		page.primitive === 'signature'
			? { href: 'signatures/', label: 'Signatures' }
			: { href: 'kems/', label: 'KEMs' }
	);
	const dt = 'font-heading text-xs font-semibold uppercase tracking-wide text-pqs-steel dark:text-pqs-bluegray';
</script>

<nav aria-label="Breadcrumb" class="font-heading text-sm text-pqs-steel dark:text-pqs-bluegray">
	<a href="{base}/{section.href}" class="underline-offset-2 hover:text-pqs-apricot hover:underline">{section.label}</a>
	<span aria-hidden="true" class="mx-1">/</span>
	<span aria-current="page">{page.name}</span>
</nav>

<h1 class="mt-2 font-heading text-3xl font-bold text-pqs-midnight dark:text-pqs-smoke">{page.name}</h1>
<p class="mt-1 text-pqs-steel dark:text-pqs-bluegray">
	{page.category} · {page.assumption}{#if page.websiteHost}
		· <a
			href={page.website}
			target="_blank"
			rel="noopener noreferrer"
			class="text-pqs-apricot underline-offset-2 hover:underline"
		>{page.websiteHost}<span class="sr-only"> (opens in a new tab)</span></a>
	{/if}
</p>

<dl class="mt-6 grid gap-4 rounded border border-pqs-ashgray bg-white p-4 sm:grid-cols-3 dark:border-pqs-steel dark:bg-pqs-midnight-mid">
	<div>
		<dt class={dt}>Programmes</dt>
		<dd class="mt-1"><ProgrammeChips chips={page.chips} link /></dd>
	</div>
	{#if page.primitive === 'signature'}
		<div>
			<dt class={dt}>Latest version</dt>
			<dd class="mt-1 text-pqs-midnight dark:text-pqs-smoke">
				{page.latest.label} · <time datetime={page.latest.date}>{fmtDate(page.latest.date)}</time>
			</dd>
		</div>
	{:else}
		<div>
			<dt class={dt}>Status</dt>
			<dd class="mt-1 text-pqs-midnight dark:text-pqs-smoke">{page.status}</dd>
		</div>
	{/if}
	<div>
		<dt class={dt}>Security flags</dt>
		<dd class="mt-1 text-pqs-midnight dark:text-pqs-smoke">{page.flagSummary}</dd>
	</div>
</dl>

{#if page.findings}
	<p class="mt-3 text-sm text-pqs-steel dark:text-pqs-bluegray">
		Third-party findings:
		<a
			href={page.findings.url}
			target="_blank"
			rel="noopener noreferrer"
			class="text-pqs-apricot underline-offset-2 hover:underline"
		>full report on {page.findings.host}<span class="sr-only"> (opens in a new tab)</span></a>.
	</p>
{/if}
