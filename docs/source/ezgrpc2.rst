ezgrpc2 documentation
=====================

The ezgrpc2 functions are built upon the idea of
looping the following, much like so in a game:


  * poll

  * update

  * render

.. autocenum:: ezgrpc2.h::ezgrpc2_status_t
   :members:
       EZGRPC2_STATUS_OK,
       EZGRPC2_STATUS_CANCELLED,
       EZGRPC2_STATUS_UNKNOWN,
       EZGRPC2_STATUS_INVALID_ARGUMENT,
       EZGRPC2_STATUS_DEADLINE_EXCEEDED,
       EZGRPC2_STATUS_NOT_FOUND,
       EZGRPC2_STATUS_ALREADY_EXISTS,
       EZGRPC2_STATUS_PERMISSION_DENIED,
       EZGRPC2_STATUS_RESOURCE_EXHAUSTED,
       EZGRPC2_STATUS_FAILED_PRECONDITION,
       EZGRPC2_STATUS_OUT_OF_RANGE,
       EZGRPC2_STATUS_UNIMPLEMENTED,
       EZGRPC2_STATUS_INTERNAL,
       EZGRPC2_STATUS_UNAVAILABLE,
       EZGRPC2_STATUS_DATA_LOSS,
       EZGRPC2_STATUS_UNAUTHENTICATED,
       EZGRPC2_STATUS_NULL
.. autocenum:: ezgrpc2.h::ezgrpc2_event_type_t
   :members: EZGRPC2_EVENT_MESSAGE, EZGRPC2_EVENT_DATALOSS, EZGRPC2_EVENT_CANCEL

.. autocstruct:: ezgrpc2.h::ezgrpc2_server_t
.. autocstruct:: ezgrpc2.h::ezgrpc2_path_t
   :members:
.. autocstruct:: ezgrpc2.h::ezgrpc2_header_t
   :members:
.. autocstruct:: ezgrpc2.h::ezgrpc2_message_t
   :members:
.. autocstruct:: ezgrpc2.h::ezgrpc2_event_t
   :members:
.. autocstruct:: ezgrpc2.h::ezgrpc2_event_message_t
   :members:
.. autocstruct:: ezgrpc2.h::ezgrpc2_event_dataloss_t
   :members:
.. autocstruct:: ezgrpc2.h::ezgrpc2_event_cancel_t
   :members:


.. autocfunction:: ezgrpc2.h::ezgrpc2_server_init
.. autocfunction:: ezgrpc2.h::ezgrpc2_server_poll
.. autocfunction:: ezgrpc2.h::ezgrpc2_session_end_stream
.. autocfunction:: ezgrpc2.h::ezgrpc2_session_end_session
.. autocfunction:: ezgrpc2.h::ezgrpc2_session_send
.. autocfunction:: ezgrpc2.h::ezgrpc2_session_find_header
