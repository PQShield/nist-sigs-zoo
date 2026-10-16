<script lang="ts">
	import type { CommonFilter } from '$lib/types';
	import { FILTER_HEADING, FILTER_OPTION, FILTER_OPTION_TEXT } from './filterStyles';

	let { filter, onchange }: { filter: CommonFilter; onchange: (patch: Partial<CommonFilter>) => void } = $props();

	const name = $props.id();
</script>

<fieldset class="mb-5 space-y-0.5">
	<legend class={FILTER_HEADING}>Parameter sets</legend>
	<label class={FILTER_OPTION}>
		<input
			type="radio"
			{name}
			class="accent-pqs-apricot"
			checked={filter.paramSets === 'recommended'}
			onchange={() => onchange({ paramSets: 'recommended' })}
		/>
		<span class={FILTER_OPTION_TEXT}>Recommended (where designers name them)</span>
	</label>
	<label class={FILTER_OPTION}>
		<input
			type="radio"
			{name}
			class="accent-pqs-apricot"
			checked={filter.paramSets === 'all'}
			onchange={() => onchange({ paramSets: 'all' })}
		/>
		<span class={FILTER_OPTION_TEXT}>All parameter sets</span>
	</label>
</fieldset>
