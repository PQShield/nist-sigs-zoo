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

<fieldset class="m-0 flex max-w-full min-w-0 flex-wrap gap-1 border-0 p-0">
	<legend class="sr-only">{label}</legend>
	{#each options as o (o.value)}
		<button
			type="button"
			aria-pressed={value === o.value}
			onclick={() => onchange(o.value)}
			class="h-9 whitespace-nowrap rounded border border-pqs-bluegray px-3 font-heading text-sm font-semibold sm:px-4 dark:border-pqs-steel {value === o.value
				? 'bg-pqs-steel text-white'
				: 'bg-white text-pqs-midnight hover:bg-pqs-smoke dark:bg-pqs-midnight-mid dark:text-pqs-smoke dark:hover:bg-pqs-steel/30'}"
		>{o.label}</button>
	{/each}
</fieldset>
