<script lang="ts">
	import { FINDING_LABELS, FINDING_ORDER, type FindingsCount } from '$lib/programmePage';
	import type { FindingsSourceYaml } from '$lib/types';

	let { source, groups }: { source: FindingsSourceYaml; groups: { label: string; count: FindingsCount }[] } = $props();
</script>

<p class="text-sm text-pqs-steel dark:text-pqs-bluegray">
	Worst recorded finding per candidate, from our data. Full reports:
	<a
		href={source.url}
		target="_blank"
		rel="noopener noreferrer"
		class="text-pqs-steel underline hover:text-pqs-apricot dark:text-pqs-apricot"
		>{source.label}<span class="sr-only"> (opens in a new tab)</span></a
	>.
</p>
<div class="mt-3 grid gap-4 md:grid-cols-2">
	{#each groups as g (g.label)}
		<div class="rounded border border-pqs-ashgray bg-white p-4 dark:border-pqs-steel dark:bg-pqs-midnight-mid">
			<h3 class="font-heading font-semibold">
				{g.label}
				<span class="font-normal text-pqs-steel dark:text-pqs-bluegray"
					>· {g.count.candidates} {g.count.candidates === 1 ? 'candidate' : 'candidates'}</span
				>
			</h3>
			{#if g.count.candidates === 0}
				<p class="mt-2 text-sm">No candidates in our data yet.</p>
			{:else}
				<ul class="mt-2 space-y-1 text-sm">
					{#each FINDING_ORDER as c (c)}
						<li><span class="font-semibold tabular-nums">{g.count[c]}</span> {FINDING_LABELS[c]}</li>
					{/each}
					<li><span class="font-semibold tabular-nums">{g.count.reports}</span> with a published findings report</li>
				</ul>
			{/if}
		</div>
	{/each}
</div>
<p class="mt-3 max-w-[80ch] text-xs text-pqs-steel dark:text-pqs-bluegray">
	Design findings concern the specified scheme; implementation findings concern the submitted code.
	{#if source.note}{source.note}{/if}
	{source.credit}
</p>
