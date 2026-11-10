<script lang="ts">
	import type { CommonFilter } from '$lib/types';
	import { FILTER_HEADING, FILTER_OPTION, FILTER_OPTION_TEXT } from './filterStyles';

	let { filter, onchange }: { filter: CommonFilter; onchange: (patch: Partial<CommonFilter>) => void } = $props();
</script>

<fieldset class="mb-5 space-y-0.5">
	<legend class={FILTER_HEADING}>Security status</legend>
	<label class={FILTER_OPTION}>
		<input
			type="checkbox"
			class="accent-pqs-apricot"
			checked={filter.hideBroken}
			onchange={(e) => onchange({ hideBroken: e.currentTarget.checked })}
		/>
		<span class={FILTER_OPTION_TEXT}>Hide broken schemes</span>
	</label>
	<label class={FILTER_OPTION}>
		<input
			type="checkbox"
			class="accent-pqs-apricot"
			checked={filter.hideImplementation}
			onchange={(e) => onchange({ hideImplementation: e.currentTarget.checked })}
		/>
		<span class={FILTER_OPTION_TEXT}>Hide schemes with implementation findings</span>
	</label>
	<label class={FILTER_OPTION}>
		<input
			type="checkbox"
			class="accent-pqs-apricot"
			checked={filter.hideWarnings}
			onchange={(e) => onchange({ hideWarnings: e.currentTarget.checked })}
		/>
		<span class={FILTER_OPTION_TEXT}>Hide warnings</span>
	</label>
</fieldset>
