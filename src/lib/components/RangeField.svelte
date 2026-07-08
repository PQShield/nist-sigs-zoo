<script lang="ts">
	interface Props {
		label: string;
		value: number;
		onchange: (v: number) => void;
	}
	let { label, value, onchange }: Props = $props();
</script>

<label class="block">
	<span class="block text-xs text-pqs-steel/70 dark:text-pqs-bluegray">{label}</span>
	<input
		type="number"
		{value}
		oninput={(e) => {
			const raw = (e.target as HTMLInputElement).value;
			if (raw === '') return; // don't treat a cleared field as 0
			const v = Number(raw);
			if (!isNaN(v)) onchange(v);
		}}
		class="w-full rounded border border-pqs-bluegray bg-pqs-smoke px-2 py-1 text-xs tabular-nums text-pqs-midnight focus:border-pqs-apricot focus:outline-none dark:border-pqs-steel dark:bg-pqs-midnight dark:text-pqs-smoke"
	/>
</label>
