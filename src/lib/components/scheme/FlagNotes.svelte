<script lang="ts">
	import type { BadgeKind } from '$lib/badges';
	import type { FlagNote } from '$lib/schemePage';
	import BadgeIcon from '../BadgeIcon.svelte';

	let { notes }: { notes: FlagNote[] } = $props();

	const TONE: Record<BadgeKind, string> = {
		'pre-quantum': 'bg-pqs-smoke dark:bg-pqs-steel/20',
		broken: 'bg-[#fdf2f2] dark:bg-[#3b1219]',
		implementation: 'bg-[#fff4ec] dark:bg-[#3a1d0c]',
		warning: 'bg-[#fdf6e7] dark:bg-[#33270a]',
		note: 'bg-pqs-smoke dark:bg-pqs-steel/20'
	};
</script>

{#if notes.length > 0}
	<section aria-labelledby="flags-heading" class="mt-8">
		<h2 id="flags-heading" class="font-heading text-xl font-bold text-pqs-midnight dark:text-pqs-smoke">
			Security flags
		</h2>
		<ul class="mt-3 space-y-2">
			{#each notes as note (note.kind + note.html)}
				<li
					class="rounded px-3 py-2.5 text-sm leading-relaxed text-pqs-midnight dark:text-pqs-smoke [&_a]:text-pqs-apricot [&_a]:underline {TONE[
						note.kind
					]}"
				>
					<BadgeIcon kind={note.kind} />
					<strong class="font-heading">{note.title}</strong>
					<span class="text-pqs-steel dark:text-pqs-bluegray"
						>({note.allSets ? 'all parameter sets' : note.sets.join(', ')})</span
					>:
					{@html note.html}
					{#if note.href}
						<a href={note.href} target="_blank" rel="noopener noreferrer"
							>Full findings report<span class="sr-only"> (opens in a new tab)</span></a
						>
					{/if}
				</li>
			{/each}
		</ul>
	</section>
{/if}
