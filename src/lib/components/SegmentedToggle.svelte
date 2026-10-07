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

<fieldset class="m-0 inline-flex max-w-full min-w-0 overflow-x-auto rounded border border-pqs-bluegray p-0 dark:border-pqs-steel">
	<legend class="sr-only">{label}</legend>
	{#each options as o (o.value)}
		<button
			type="button"
			aria-pressed={value === o.value}
			onclick={() => onchange(o.value)}
			class="h-9 shrink-0 whitespace-nowrap px-3 font-heading sm:px-4 text-sm font-semibold {value === o.value
				? 'bg-pqs-steel text-white'
				: 'bg-white text-pqs-midnight hover:bg-pqs-smoke dark:bg-pqs-midnight-mid dark:text-pqs-smoke dark:hover:bg-pqs-steel/30'}"
		>{o.label}</button>
	{/each}
</fieldset>
