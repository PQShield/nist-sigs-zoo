<script lang="ts">
	import type { BadgeKind } from '$lib/badges';

	let { kind }: { kind: BadgeKind } = $props();
</script>

<svg width="16" height="16" viewBox="0 0 24 24" aria-hidden="true" class="inline-block align-middle">
	{#if kind === 'pre-quantum'}
		<rect x="2" y="2" width="20" height="20" rx="4" fill="#6B7280" />
		<path d="M7 17L17 7" stroke="#ffffff" stroke-width="2.5" stroke-linecap="round" />
	{:else if kind === 'broken'}
		<circle cx="12" cy="12" r="10" class="fill-[#B91C1C] dark:fill-[#EF4444]" />
		<path d="M8 8l8 8M16 8l-8 8" stroke="#ffffff" stroke-width="2.5" stroke-linecap="round" />
	{:else if kind === 'implementation'}
		<rect x="2" y="2" width="20" height="20" rx="5" fill="#C2410C" />
		<text
			x="12"
			y="16.5"
			text-anchor="middle"
			font-size="12"
			font-weight="700"
			fill="#ffffff"
			font-family="ui-monospace, monospace">{'{}'}</text
		>
	{:else if kind === 'warning'}
		<path d="M12 2.5L23 21.5H1Z" fill="#B45309" />
		<path d="M12 9v5.5M12 17.5v.5" stroke="#ffffff" stroke-width="2.5" stroke-linecap="round" />
	{:else}
		<circle cx="12" cy="12" r="10" class="fill-[#064058] dark:fill-[#5B9BB8]" />
		<path d="M12 11v6M12 7.5v.5" stroke="#ffffff" stroke-width="2.5" stroke-linecap="round" />
	{/if}
</svg>
