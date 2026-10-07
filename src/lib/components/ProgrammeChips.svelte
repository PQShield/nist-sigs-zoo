<script lang="ts">
	import { base } from '$app/paths';
	import type { Chip, ChipTone } from '$lib/badges';

	let { chips, link = false }: { chips: Chip[]; link?: boolean } = $props();

	const TONE: Record<ChipTone, string> = {
		standard: 'bg-[#fbe9d2] text-[#7a4606] dark:bg-pqs-apricot/20 dark:text-pqs-apricot',
		new: 'bg-[#fdf3e6] text-[#8a5208] dark:bg-pqs-apricot/10 dark:text-pqs-apricot-light',
		plain: 'bg-pqs-ashgray text-pqs-midnight dark:bg-pqs-steel/40 dark:text-pqs-smoke'
	};
</script>

<span class="flex flex-wrap gap-1">
	{#each chips as chip (chip.label)}
		{#if link && chip.programme}
			<a
				href="{base}/programmes/{chip.programme}/"
				class="whitespace-nowrap rounded px-1.5 py-px text-[11px] font-bold hover:underline {TONE[chip.tone]}">{chip.label}</a
			>
		{:else}
			<span class="whitespace-nowrap rounded px-1.5 py-px text-[11px] font-bold {TONE[chip.tone]}">{chip.label}</span>
		{/if}
	{/each}
</span>
