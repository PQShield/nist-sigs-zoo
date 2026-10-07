<script lang="ts">
	import type { ParamSetTableModel } from '$lib/schemePage';
	import BadgeIcon from '../BadgeIcon.svelte';

	let { table, caption }: { table: ParamSetTableModel; caption: string } = $props();
</script>

<div class="relative mt-3 overflow-x-auto rounded border border-pqs-ashgray shadow-sm dark:border-pqs-steel">
	<table class="min-w-full text-sm">
		<caption class="sr-only">{caption}</caption>
		<thead>
			<tr class="bg-pqs-steel font-heading text-xs text-white">
				<th scope="col" class="whitespace-nowrap px-3 py-2.5 text-left font-semibold">Parameter set</th>
				<th scope="col" class="whitespace-nowrap px-3 py-2.5 text-right font-semibold">Level</th>
				{#each table.headers as header (header)}
					<th scope="col" class="whitespace-nowrap px-3 py-2.5 text-right font-semibold">{header}</th>
				{/each}
			</tr>
		</thead>
		<tbody class="divide-y divide-pqs-ashgray bg-white dark:divide-pqs-steel dark:bg-pqs-midnight-mid">
			{#each table.rows as row (row.key)}
				<tr>
					<th scope="row" class="whitespace-nowrap px-3 py-1.5 text-left font-mono text-xs font-normal">
						{row.name}
						{#if row.recommendedTag}
							<span class="ml-1 rounded bg-pqs-ashgray px-1.5 py-px font-heading text-[11px] font-bold text-pqs-midnight dark:bg-pqs-steel/40 dark:text-pqs-smoke">Recommended</span>
						{/if}
						{#each row.badges as badge (badge.kind)}
							<span class="ml-1" title={badge.title}><BadgeIcon kind={badge.kind} /><span class="sr-only">{badge.title}</span></span>
						{/each}
					</th>
					<td class="whitespace-nowrap px-3 py-1.5 text-right tabular-nums text-pqs-steel dark:text-pqs-bluegray">{row.level}</td>
					{#each row.cells as cell, i (i)}
						<td class="whitespace-nowrap px-3 py-1.5 text-right tabular-nums">
							{#if cell.kind === 'estimated'}
								<span class="underline decoration-wavy decoration-pqs-scarlet" title={cell.title}>{cell.text}<span class="sr-only"> ({cell.title})</span></span>
							{:else if cell.kind === 'missing'}
								<span class="text-pqs-bluegray" title={cell.title}>—<span class="sr-only"> {cell.title}</span></span>
							{:else}
								{cell.text}
							{/if}
						</td>
					{/each}
				</tr>
			{/each}
		</tbody>
	</table>
</div>
