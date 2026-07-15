<script lang="ts">
	interface Props {
		broken: false | string;
		warning: false | string;
		info: false | string;
		classical: boolean;
	}
	let { broken, warning, info, classical }: Props = $props();
	let expanded = $state(false);
	const msgId = $props.id();

	function toggle() {
		expanded = !expanded;
	}
</script>

{#if classical}
	<button class="badge" aria-label="Pre-quantum scheme (not quantum-resistant)" aria-expanded={expanded} aria-controls={expanded ? msgId : undefined} onclick={toggle}>💣</button>
	{#if expanded}
		<span class="msg text-pqs-steel/80 dark:text-pqs-bluegray/80" id={msgId}>Pre-quantum: {broken}</span>
	{/if}
{:else if broken}
	<button class="badge" aria-label="Broken scheme" aria-expanded={expanded} aria-controls={expanded ? msgId : undefined} onclick={toggle}>🧨</button>
	{#if expanded}
		<span class="msg msg-danger" id={msgId}>Broken: {broken}</span>
	{/if}
{:else if warning}
	<button class="badge" aria-label="Security warning" aria-expanded={expanded} aria-controls={expanded ? msgId : undefined} onclick={toggle}>⚠️</button>
	{#if expanded}
		<span class="msg msg-warning" id={msgId}>Warning: {warning}</span>
	{/if}
{:else if info}
	<button class="badge" aria-label="Security note" aria-expanded={expanded} aria-controls={expanded ? msgId : undefined} onclick={toggle}>ℹ️</button>
	{#if expanded}
		<span class="msg text-pqs-steel dark:text-pqs-bluegray" id={msgId}>Note: {info}</span>
	{/if}
{/if}

<style>
	button.badge {
		background: none;
		border: none;
		padding: 0;
		cursor: pointer;
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
</style>
