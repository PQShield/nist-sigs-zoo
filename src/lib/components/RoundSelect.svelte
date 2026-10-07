<script lang="ts">
	import { roundStore, type Round } from '$lib/roundStore';

	let { disabled = false }: { disabled?: boolean } = $props();

	const id = $props.id();

	const OPTIONS: { value: Round; label: string }[] = [
		{ value: 'latest', label: 'Latest specs' },
		{ value: 'round-3', label: 'Round 3' },
		{ value: 'round-2', label: 'Round 2' },
		{ value: 'round-1', label: 'Round 1' }
	];
</script>

<label for={id} class="sr-only">On-ramp round</label>
<select
	{id}
	{disabled}
	class="mb-1.5 mt-0.5 h-8 w-full rounded border border-pqs-bluegray bg-white px-2 text-sm text-pqs-midnight disabled:opacity-50 dark:border-pqs-steel dark:bg-pqs-midnight dark:text-pqs-smoke"
	value={$roundStore}
	onchange={(e) => roundStore.set(e.currentTarget.value as Round)}
>
	{#each OPTIONS as opt (opt.value)}
		<option value={opt.value}>{opt.label}</option>
	{/each}
</select>
