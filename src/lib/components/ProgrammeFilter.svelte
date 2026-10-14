<script lang="ts">
	import type { Snippet } from 'svelte';
	import { PROGRAMME_GROUP_LABELS } from '$lib/compareFilters';
	import type { CommonFilter, ProgrammeGroup } from '$lib/types';
	import { FILTER_HEADING, FILTER_OPTION, FILTER_OPTION_TEXT } from './filterStyles';

	interface Props {
		filter: CommonFilter;
		groups: ProgrammeGroup[];
		onchange: (patch: Partial<CommonFilter>) => void;
		/** Rendered under "NIST on-ramp"; gets whether that programme is ticked. */
		onramp?: Snippet<[boolean]>;
	}
	let { filter, groups, onchange, onramp }: Props = $props();

	function toggle(group: ProgrammeGroup) {
		const next = new Set(filter.programmes);
		if (next.has(group)) next.delete(group);
		else next.add(group);
		onchange({ programmes: next });
	}
</script>

<fieldset class="mb-5">
	<legend class={FILTER_HEADING}>Programme</legend>
	<div class="space-y-0.5">
		{#each groups as group (group)}
			<label class={FILTER_OPTION}>
				<input type="checkbox" class="accent-pqs-apricot" checked={filter.programmes.has(group)} onchange={() => toggle(group)} />
				<span class={FILTER_OPTION_TEXT}>{PROGRAMME_GROUP_LABELS[group]}</span>
			</label>
			{#if group === 'nist-onramp' && onramp}
				<div class="ml-5">{@render onramp(filter.programmes.has('nist-onramp'))}</div>
			{/if}
		{/each}
	</div>
</fieldset>
