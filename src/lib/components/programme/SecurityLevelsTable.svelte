<script lang="ts">
	import type { SecurityLevelTableYaml } from '$lib/types';

	let { table }: { table: SecurityLevelTableYaml } = $props();
</script>

<p class="max-w-[80ch] text-sm text-pqs-steel dark:text-pqs-bluegray">{table.intro}</p>
<div class="mt-3 max-w-3xl overflow-x-auto rounded border border-pqs-ashgray bg-white dark:border-pqs-steel dark:bg-pqs-midnight-mid">
	<table class="min-w-full text-sm">
		<thead>
			<tr class="bg-pqs-steel font-heading text-xs text-white">
				{#each table.columns as c (c)}
					<th scope="col" class="px-3 py-2 text-left font-semibold">{c}</th>
				{/each}
			</tr>
		</thead>
		<tbody class="divide-y divide-pqs-ashgray dark:divide-pqs-steel">
			{#each table.rows as row, i (i)}
				<tr>
					{#each row as cell, j (j)}
						{#if j === 0}
							<th scope="row" class="px-3 py-2 text-left font-heading font-semibold">{cell}</th>
						{:else}
							<td class="px-3 py-2">{cell}</td>
						{/if}
					{/each}
				</tr>
			{/each}
		</tbody>
	</table>
</div>
{#if table.note}
	<p class="mt-2 max-w-[80ch] text-xs text-pqs-steel dark:text-pqs-bluegray">{table.note}</p>
{/if}
