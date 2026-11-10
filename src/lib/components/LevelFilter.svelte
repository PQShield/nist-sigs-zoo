<script lang="ts">
	import { NGCC_LEVELS } from '$lib/constants';
	import type { CommonFilter, NgccLevel, NistLevel } from '$lib/types';
	import { FILTER_HEADING, FILTER_OPTION, FILTER_OPTION_TEXT, FILTER_SUBHEADING } from './filterStyles';

	let { filter, onchange }: { filter: CommonFilter; onchange: (patch: Partial<CommonFilter>) => void } = $props();

	const NIST_ORDER: NistLevel[] = [1, 2, 3, 4, 5, 'Pre-Quantum'];

	function toggleNist(level: NistLevel) {
		const next = new Set(filter.levels);
		if (next.has(level)) next.delete(level);
		else next.add(level);
		onchange({ levels: next });
	}

	function toggleNgcc(level: NgccLevel) {
		const next = new Set(filter.ngccLevels);
		if (next.has(level)) next.delete(level);
		else next.add(level);
		onchange({ ngccLevels: next });
	}
</script>

<fieldset class="mb-5">
	<legend class={FILTER_HEADING}>Security level</legend>
	<fieldset class="space-y-0.5">
		<legend class={FILTER_SUBHEADING}>NIST category</legend>
		{#each NIST_ORDER as level (level)}
			<label class={FILTER_OPTION}>
				<input
					type="checkbox"
					class="accent-pqs-apricot"
					checked={filter.levels.has(level)}
					onchange={() => toggleNist(level)}
				/>
				<span class={FILTER_OPTION_TEXT}>{level === 'Pre-Quantum' ? 'Pre-quantum' : `Level ${level}`}</span>
			</label>
		{/each}
	</fieldset>
	<fieldset class="mt-2 space-y-0.5">
		<legend class={FILTER_SUBHEADING}>NGCC target (classical bits)</legend>
		{#each NGCC_LEVELS as level (level)}
			<label class={FILTER_OPTION}>
				<input
					type="checkbox"
					class="accent-pqs-apricot"
					checked={filter.ngccLevels.has(level)}
					onchange={() => toggleNgcc(level)}
				/>
				<span class={FILTER_OPTION_TEXT}>{level}-bit</span>
			</label>
		{/each}
	</fieldset>
</fieldset>
