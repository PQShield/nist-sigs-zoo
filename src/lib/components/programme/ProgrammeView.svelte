<script lang="ts">
	import { untrack } from 'svelte';
	import { base } from '$app/paths';
	import { SITE_NAME } from '$lib/constants';
	import { allKemData } from '$lib/kemSchemeData';
	import { allSchemeData } from '$lib/schemeData';
	import {
		CURRENT_VIEW,
		findingsSummary,
		kemMembers,
		programmeFacts,
		programmeKemRows,
		programmeSignatureRows,
		signatureMembers
	} from '$lib/programmePage';
	import type { ProgrammeYaml } from '$lib/types';
	import SegmentedToggle from '../SegmentedToggle.svelte';
	import FindingsSummary from './FindingsSummary.svelte';
	import KemCandidates from './KemCandidates.svelte';
	import SignatureCandidates from './SignatureCandidates.svelte';
	import ProgrammeTimeline from './ProgrammeTimeline.svelte';
	import SecurityLevelsTable from './SecurityLevelsTable.svelte';

	let { programme }: { programme: ProgrammeYaml } = $props();

	// The route remounts this component per programme ({#key}), so one read of the prop is enough.
	const p = untrack(() => programme);
	const hasSigs = p.primitives.includes('signature');
	const hasKems = p.primitives.includes('kem');
	const sigCount = signatureMembers(p.id, allSchemeData).length;
	const kemCount = kemMembers(p.id, allKemData).length;
	const facts = programmeFacts(p, { signature: sigCount, kem: kemCount });
	const navLabel = p.nav_label ?? p.short;
	const heading = p.name.includes(p.short) ? p.name : `${p.name} (${p.short})`;
	const findingGroups = [
		...(hasSigs
			? [{ label: 'Signatures', count: findingsSummary(programmeSignatureRows(p.id, allSchemeData, CURRENT_VIEW).parameterSets) }]
			: []),
		...(hasKems ? [{ label: 'KEMs', count: findingsSummary(programmeKemRows(p.id, allKemData)) }] : [])
	];

	type Kind = 'signature' | 'kem';
	const kindOptions: { value: Kind; label: string }[] = [
		...(hasSigs ? [{ value: 'signature' as Kind, label: `Signatures (${sigCount})` }] : []),
		...(hasKems ? [{ value: 'kem' as Kind, label: `KEMs (${kemCount})` }] : [])
	];
	let kind = $state<Kind>(hasSigs ? 'signature' : 'kem');

	const LINK = 'text-pqs-steel underline hover:text-pqs-apricot dark:text-pqs-apricot';
	const H2 = 'mb-3 font-heading text-2xl font-bold';
</script>

<svelte:head>
	<title>{navLabel} programme · {SITE_NAME}</title>
</svelte:head>

<div class="mx-auto max-w-screen-2xl space-y-12 px-6 py-8">
	<header>
		<nav aria-label="Breadcrumb" class="mb-4 font-heading text-sm">
			<ol class="flex flex-wrap gap-2 text-pqs-steel dark:text-pqs-bluegray">
				<li><a href="{base}/programmes/" class={LINK}>Programmes</a></li>
				<li aria-hidden="true">/</li>
				<li aria-current="page" class="font-semibold text-pqs-midnight dark:text-white">{navLabel}</li>
			</ol>
		</nav>
		<div class="grid gap-8 lg:grid-cols-[1fr_320px]">
			<div class="border-l-4 border-pqs-apricot pl-4">
				<h1 class="font-heading text-3xl font-bold text-pqs-midnight dark:text-white">{heading}</h1>
				{#if p.description}
					<p class="mt-3 max-w-[72ch] text-pqs-steel dark:text-pqs-bluegray">{p.description}</p>
				{/if}
				{#if p.links?.length}
					<ul class="mt-4 flex flex-wrap gap-x-5 gap-y-2 font-heading text-sm font-semibold">
						{#each p.links as l (l.url)}
							<li><a href={l.url} target="_blank" rel="noopener noreferrer" class={LINK}>{l.label} ↗</a></li>
						{/each}
					</ul>
				{/if}
			</div>
			<dl class="grid content-start gap-3 rounded border border-pqs-ashgray bg-white p-4 text-sm dark:border-pqs-steel dark:bg-pqs-midnight-mid">
				{#each facts as f (f.term)}
					<div>
						<dt class="font-heading text-xs font-semibold uppercase tracking-wide text-pqs-steel dark:text-pqs-bluegray">{f.term}</dt>
						<dd>{f.detail}</dd>
					</div>
				{/each}
			</dl>
		</div>
	</header>

	{#if p.timeline?.length}
		<section aria-labelledby="timeline-h">
			<h2 id="timeline-h" class={H2}>Timeline</h2>
			<ProgrammeTimeline entries={p.timeline} />
		</section>
	{/if}

	{#if p.security_levels}
		<section aria-labelledby="levels-h">
			<h2 id="levels-h" class={H2}>Security levels</h2>
			<SecurityLevelsTable table={p.security_levels} />
		</section>
	{/if}

	{#if p.findings}
		<section aria-labelledby="findings-h">
			<h2 id="findings-h" class={H2}>Published findings</h2>
			<FindingsSummary source={p.findings} groups={findingGroups} />
		</section>
	{/if}

	<section aria-labelledby="candidates-h" class="space-y-4">
		<h2 id="candidates-h" class={H2}>{p.candidates_heading ?? 'Schemes'}</h2>
		{#if kindOptions.length > 1}
			<SegmentedToggle label="Primitive" options={kindOptions} value={kind} onchange={(k) => (kind = k)} />
		{:else}
			<p class="font-heading text-sm font-semibold">{kindOptions[0].label}</p>
		{/if}
		{#if kind === 'signature'}
			<SignatureCandidates programme={p} />
		{:else}
			<KemCandidates programme={p} />
		{/if}
	</section>
</div>
