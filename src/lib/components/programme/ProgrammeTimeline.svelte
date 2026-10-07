<script lang="ts">
	import { fmtDate } from '$lib/dates';
	import type { ProgrammeTimelineEntry } from '$lib/types';

	let { entries }: { entries: ProgrammeTimelineEntry[] } = $props();
</script>

<ol class="grid gap-3 sm:grid-cols-2 lg:grid-cols-5">
	{#each entries as e, i (i)}
		<li
			aria-current={e.current ? 'step' : undefined}
			class="rounded border-t-4 bg-white px-3 py-2 dark:bg-pqs-midnight-mid {e.current
				? 'border-pqs-apricot'
				: 'border-pqs-ashgray dark:border-pqs-steel'}"
		>
			<p class="font-heading text-sm font-semibold">
				{e.label}{#if e.note}<span class="font-normal">: {e.note}</span>{/if}
			</p>
			{#if e.current || e.date}
				<p class="text-xs text-pqs-steel dark:text-pqs-bluegray">
					{#if e.current}<span class="font-semibold text-pqs-midnight dark:text-pqs-apricot">Current</span>{#if e.date}{' · '}{/if}{/if}
					{#if e.date}<time datetime={e.date}>{fmtDate(e.date)}</time>{/if}
				</p>
			{/if}
		</li>
	{/each}
</ol>
