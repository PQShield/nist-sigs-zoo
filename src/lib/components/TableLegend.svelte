<script lang="ts">
	import { BADGE_LEGEND, type BadgeKind } from '$lib/badges';
	import { CPU_GHZ_LABEL } from '$lib/constants';
	import BadgeIcon from './BadgeIcon.svelte';

	const KINDS: BadgeKind[] = ['pre-quantum', 'broken', 'implementation', 'warning', 'note'];
</script>

<div
	class="flex flex-wrap items-center gap-x-4 gap-y-1.5 border-t border-pqs-ashgray bg-pqs-smoke px-3 py-2 text-xs text-pqs-steel/80 dark:border-pqs-steel dark:bg-pqs-midnight-mid dark:text-pqs-bluegray/80"
>
	<strong class="font-heading text-pqs-steel dark:text-pqs-bluegray">Legend</strong>
	{#each KINDS as kind (kind)}
		<span class="inline-flex items-center gap-1"><BadgeIcon {kind} />{BADGE_LEGEND[kind]}</span>
	{/each}
	<span
		><span class="underline decoration-pqs-scarlet decoration-wavy">value</span> estimated from cycle counts at {CPU_GHZ_LABEL}</span
	>
	<span>— not yet benchmarked</span>
	<span>timeout / DNF benchmark did not finish (too slow / failed)</span>
	<span>Select an icon for details</span>
</div>
