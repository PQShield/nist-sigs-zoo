<script lang="ts">
	import type { LegendGroup } from '$lib/chartLegend';

	let { groups, footnote }: { groups: LegendGroup[]; footnote?: string } = $props();
</script>

<div
	role="group"
	aria-label="Legend"
	class="flex flex-wrap gap-x-8 gap-y-3 text-xs text-pqs-midnight dark:text-pqs-smoke lg:w-44 lg:shrink-0 lg:flex-col lg:flex-nowrap lg:gap-5"
>
	{#each groups as group (group.title)}
		<div>
			<h3 class="mb-1.5 font-heading text-xs font-bold">{group.title}</h3>
			<ul class="flex flex-wrap gap-x-3.5 gap-y-1.5 lg:flex-col">
				{#each group.items as item (item.label)}
					<li class="flex items-center gap-1.5">
						{#if item.shape}
							<svg width="12" height="12" viewBox="-1.1 -1.1 2.2 2.2" aria-hidden="true">
								<path d={item.shape} class="fill-pqs-steel dark:fill-pqs-bluegray" />
							</svg>
						{:else if item.colour}
							<span class="inline-block h-2.5 w-2.5 rounded-sm" style:background={item.colour} aria-hidden="true"></span>
						{:else if item.ring}
							<span class="inline-block h-3 w-3 rounded-full border-2" style:border-color={item.ring} aria-hidden="true"></span>
						{/if}
						{item.label}
					</li>
				{/each}
			</ul>
			{#if group.note}
				<p class="mt-1 text-pqs-steel dark:text-pqs-bluegray">{group.note}</p>
			{/if}
		</div>
	{/each}
	{#if footnote}
		<p class="text-pqs-steel dark:text-pqs-bluegray">{footnote}</p>
	{/if}
</div>
