<script lang="ts">
	import { roundStore, type Round } from '$lib/roundStore';

	const OPTIONS: { value: Round; label: string }[] = [
		{ value: 'latest', label: 'Latest specs' },
		{ value: 'round-3', label: 'Round 3' },
		{ value: 'round-2', label: 'Round 2' },
		{ value: 'round-1', label: 'Round 1' }
	];
</script>

<label class="inline-flex items-center gap-2 font-heading text-sm text-pqs-steel dark:text-pqs-bluegray">
	Dataset
	<select
		class="rounded border border-pqs-bluegray bg-white px-2 py-1 text-pqs-midnight dark:border-pqs-steel dark:bg-pqs-midnight-mid dark:text-pqs-smoke"
		value={$roundStore}
		onchange={(e) => roundStore.set(e.currentTarget.value as Round)}
	>
		{#each OPTIONS as opt (opt.value)}
			<option value={opt.value}>{opt.label}</option>
		{/each}
	</select>
</label>
