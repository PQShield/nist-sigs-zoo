<script lang="ts" generics="T extends string | number">
	let {
		label,
		options,
		value,
		onchange
	}: {
		label: string;
		options: { value: T; label: string }[];
		value: T;
		onchange: (value: T) => void;
	} = $props();
</script>

<fieldset class="inline-flex flex-wrap overflow-hidden rounded border border-pqs-bluegray dark:border-pqs-steel">
	<legend class="sr-only">{label}</legend>
	{#each options as o (o.value)}
		<button
			type="button"
			aria-pressed={value === o.value}
			onclick={() => onchange(o.value)}
			class="h-9 px-4 font-heading text-sm font-semibold {value === o.value
				? 'bg-pqs-steel text-white'
				: 'bg-white text-pqs-midnight hover:bg-pqs-smoke dark:bg-pqs-midnight-mid dark:text-pqs-smoke dark:hover:bg-pqs-steel/30'}"
		>{o.label}</button>
	{/each}
</fieldset>
