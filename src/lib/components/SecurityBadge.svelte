<script lang="ts">
	interface Props {
		broken: false | string;
		warning: false | string;
		info: false | string;
		classical: boolean;
	}
	let { broken, warning, info, classical }: Props = $props();
</script>

{#if classical}
	<span aria-label="Pre-quantum scheme (not quantum-resistant)" title="Pre-quantum: {broken}">💣</span>
{:else if broken}
	<span aria-label="Broken scheme" title="Broken: {broken}">🧨</span>
{:else if warning}
	<span aria-label="Security warning" title="Warning: {warning}">⚠️</span>
{:else if info}
	<span aria-label="Security note" title="Note: {info}">ℹ️</span>
{/if}
