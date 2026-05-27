<script lang="ts">
	interface Props {
		broken: false | string;
		warning: false | string;
		info: false | string;
		classical: boolean;
	}
	let { broken, warning, info, classical }: Props = $props();
	let expanded = $state(false);

	function toggle(e: MouseEvent | KeyboardEvent) {
		if (e instanceof KeyboardEvent && e.key !== 'Enter' && e.key !== ' ') return;
		e.preventDefault();
		expanded = !expanded;
	}
</script>

{#if classical}
	<button class="badge" aria-label="Pre-quantum scheme (not quantum-resistant)" aria-expanded={expanded} onclick={toggle}>💣</button>
	{#if expanded}
		<span class="msg msg-muted">Pre-quantum: {broken}</span>
	{/if}
{:else if broken}
	<button class="badge" aria-label="Broken scheme" aria-expanded={expanded} onclick={toggle}>🧨</button>
	{#if expanded}
		<span class="msg msg-danger">Broken: {broken}</span>
	{/if}
{:else if warning}
	<button class="badge" aria-label="Security warning" aria-expanded={expanded} onclick={toggle}>⚠️</button>
	{#if expanded}
		<span class="msg msg-warning">Warning: {warning}</span>
	{/if}
{:else if info}
	<button class="badge" aria-label="Security note" aria-expanded={expanded} onclick={toggle}>ℹ️</button>
	{#if expanded}
		<span class="msg msg-info">Note: {info}</span>
	{/if}
{/if}

<style>
	button.badge {
		background: none;
		border: none;
		padding: 0;
		cursor: help;
		font: inherit;
		vertical-align: baseline;
	}
	span.msg {
		display: block;
		white-space: normal;
		font-size: 0.7rem;
		max-width: 24rem;
		margin-top: 0.125rem;
	}
	.msg-danger { color: var(--color-pqs-scarlet); }
	.msg-warning { color: var(--color-pqs-tangerine); }
	.msg-muted { color: var(--color-pqs-steel); opacity: 0.7; }
	.msg-info { color: var(--color-pqs-bluegray); }
</style>
