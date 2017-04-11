/*
 * Copyright 2017, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of the copyright holder nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * template_vfork.c -- templates for vfork() syscall in full-follow-fork mode
 */

/*
 * kprobe__SYSCALL_NAME -- SYSCALL_NAME() entry handler
 */
int
kprobe__SYSCALL_NAME(struct pt_regs *ctx)
{
	struct data_entry_t ev;
	u64 pid_tid = bpf_get_current_pid_tgid();

	PID_CHECK_HOOK

	ev.type = E_SC_ENTRY;
	ev.start_ts_nsec = bpf_ktime_get_ns();

	ev.packet_type = 0; /* No additional packets */
	ev.sc_id = SYSCALL_NR; /* SysCall ID */
	ev.pid_tid = pid_tid;

	ev.args[0] = PT_REGS_PARM1(ctx);
	ev.args[1] = PT_REGS_PARM2(ctx);
	ev.args[2] = PT_REGS_PARM3(ctx);
	ev.args[3] = PT_REGS_PARM4(ctx);
	ev.args[4] = PT_REGS_PARM5(ctx);
	ev.args[5] = PT_REGS_PARM6(ctx);

	events.perf_submit(ctx, &ev, offsetof(struct data_entry_t, args));

	return 0;
};

/*
 * kretprobe__SYSCALL_NAME -- SYSCALL_NAME() exit handler
 */
int
kretprobe__SYSCALL_NAME(struct pt_regs *ctx)
{
	struct data_exit_t ev;

	u64 cur_nsec = bpf_ktime_get_ns();
	u64 pid_tid = bpf_get_current_pid_tgid();

	PID_CHECK_HOOK

	ev.type = E_SC_EXIT;
	ev.packet_type = 0; /* No additional packets */
	ev.sc_id = SYSCALL_NR; /* SysCall ID */
	ev.pid_tid = pid_tid;
	ev.finish_ts_nsec = cur_nsec;
	ev.ret = PT_REGS_RC(ctx);

	if (0 < ev.ret) {
		u64 one = 1;
		children_map.update(&ev.ret, &one);
	}

	events.perf_submit(ctx, &ev, sizeof(struct data_exit_t));

	return 0;
}