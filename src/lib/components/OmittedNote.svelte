<script lang="ts">
	import { omittedNote } from '$lib/chartData';

	/** `count` rows not shown, `dnf` of them because their benchmark did not finish. */
	let { count, dnf = 0 }: { count: number; dnf?: number } = $props();
</script>

{#if count > 0}
	<p
		role="note"
		class="mt-2 rounded border border-dashed border-pqs-violet bg-pqs-violet-pale px-2.5 py-1.5 text-xs text-pqs-violet-ink dark:bg-pqs-violet-deep/40 dark:text-pqs-violet-mist"
	>
		{omittedNote(count - dnf, dnf)}
		{#if count > dnf}They will appear once they are added to our benchmark harness.{/if}
	</p>
{/if}
