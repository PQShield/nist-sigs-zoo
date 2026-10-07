<script lang="ts">
	import type { Snippet } from 'svelte';

	let { summary, children }: { summary: string; children: Snippet } = $props();

	let open = $state(false);
	const bodyId = $props.id();
</script>

<div class="w-full shrink-0 lg:sticky lg:top-4 lg:max-h-[calc(100vh-2rem)] lg:w-64 lg:self-start lg:overflow-y-auto">
	<button
		type="button"
		class="flex min-h-11 w-full items-center gap-3 rounded border border-pqs-ashgray bg-white px-3 text-left dark:border-pqs-steel dark:bg-pqs-midnight-mid lg:hidden"
		aria-expanded={open}
		aria-controls={bodyId}
		onclick={() => (open = !open)}
	>
		<span class="font-heading font-bold text-pqs-midnight dark:text-white">Filters</span>
		<span class="flex-1 text-sm text-pqs-steel dark:text-pqs-bluegray">{summary}</span>
		<svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" aria-hidden="true" class="transition-transform {open ? 'rotate-180' : ''}"><path d="M6 9l6 6 6-6" /></svg>
	</button>
	<div id={bodyId} class={open ? 'mt-3 lg:mt-0' : 'hidden lg:block'}>
		{@render children()}
	</div>
</div>
