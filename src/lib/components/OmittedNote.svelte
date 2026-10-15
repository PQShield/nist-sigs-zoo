<script lang="ts">
	import { omittedNote } from '$lib/chartData';

	/** `count` rows not shown: `noCode` of them have no implementation, `dnf` of them did not finish. */
	let { count, dnf = 0, noCode = 0 }: { count: number; dnf?: number; noCode?: number } = $props();
	const pending = $derived(count - dnf - noCode);
</script>

{#if count > 0}
	<p
		role="note"
		class="mt-2 rounded border border-dashed border-pqs-violet bg-pqs-violet-pale px-2.5 py-1.5 text-xs text-pqs-violet-ink dark:bg-pqs-violet-deep/40 dark:text-pqs-violet-mist"
	>
		{omittedNote(pending, dnf, noCode)}
		{#if pending > 0}They will appear once they are added to our benchmark harness.{/if}
	</p>
{/if}
