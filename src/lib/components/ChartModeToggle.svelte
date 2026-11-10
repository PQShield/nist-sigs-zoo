<script lang="ts">
	import type { ChartMode } from '$lib/types';

	let { mode, onchange }: { mode: ChartMode; onchange: (mode: ChartMode) => void } = $props();

	const MODES: { value: ChartMode; label: string }[] = [
		{ value: 'sizes', label: 'Sizes' },
		{ value: 'speed', label: 'Speed' }
	];
</script>

<div
	role="group"
	aria-label="Chart axes"
	class="inline-flex overflow-hidden rounded border border-pqs-bluegray dark:border-pqs-steel"
>
	{#each MODES as m (m.value)}
		<button
			type="button"
			aria-pressed={mode === m.value}
			onclick={() => onchange(m.value)}
			class="h-9 px-4 font-heading text-sm font-semibold {mode === m.value
				? 'bg-pqs-steel text-white'
				: 'bg-white text-pqs-midnight hover:bg-pqs-smoke dark:bg-pqs-midnight-mid dark:text-pqs-smoke dark:hover:bg-pqs-steel/30'}"
			>{m.label}</button
		>
	{/each}
</div>
